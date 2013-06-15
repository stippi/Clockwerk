/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DownloadJob.h"

#include <stdio.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <Message.h>

#include "common_constants.h"

#include "AutoDeleter.h"
#include "Logger.h"
#include "RequestConnection.h"
#include "RequestMessageCodes.h"
#include "SHA256.h"

#include "StatusOutput.h"


static Logger sLog("network.jobs.DownloadJob");


// constructor
DownloadJob::DownloadJob(const char* directory, const BString& serverID,
		int32 version, BHandler* handler, BMessage* message, bool getData)
	: InformingJob(handler, message),
	  fPath(directory),
	  fServerID(serverID),
	  fVersion(version),
	  fData(NULL),
	  fGetData(getData)
{
}

// constructor
DownloadJob::DownloadJob(const BString& serverID, int32 version, BDataIO* data,
		BHandler* handler, BMessage* message)
	: InformingJob(handler, message),
	  fPath("/tmp"),
	  fServerID(serverID),
	  fVersion(version),
	  fData(data),
	  fGetData(data)
{
}

// destructor
DownloadJob::~DownloadJob()
{
}

// Execute
status_t
DownloadJob::Execute(JobConnection* jobConnection)
{
	LOG_INFO("Execute(): object: %s, version: %ld, get data: %d (%p)\n",
		fServerID.String(), fVersion, fGetData, fData);

	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection) {
		return fError = ENOTCONN;
	}

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	fError = B_OK;

	// allocate buffer
	enum {
		BUFFER_SIZE	= 1024 * 1024
	};
	void* buffer = NULL;
	if (fGetData) {
		buffer = malloc(BUFFER_SIZE);
		if (!buffer) {
			statusOutput->PrintErrorMessage("DownloadJob: Out of memory\n");
			LOG_ERROR("Execute(): Out of memory\n");
			return fError = B_NO_MEMORY;
		}
	}
	MemoryDeleter _(buffer);

	// tell the user, that we're starting
	if (fGetData) {
		statusOutput->PrintInfoMessage(
			"DownloadJob: '%s'", fServerID.String());
	} else {
		statusOutput->PrintInfoMessage(
			"DownloadJob: (meta data) '%s'", fServerID.String());
	}

	entry_ref ref;
	BEntry entry;
	BFile file;
	off_t skipData = 0;
	BString existingFileSha256String;
	SHA256 realSHA256;
	bool skippedDataBecauseSHAMatches = false;

	if (fGetData && fData == NULL) {	
		// prepare the file
		char fileName[B_FILE_NAME_LENGTH];
		sprintf(fileName, "%s:%ld", fServerID.String(), fVersion);
	
		BPath filePath(fPath.Path(), fileName);
		fError = entry.SetTo(filePath.Path(), true);
		if (fError == B_OK)
			fError = entry.GetRef(&ref);
		if (fError != B_OK) {
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: Failed construct file entry: %s\n",
				strerror(fError));
			LOG_ERROR("Execute(): Failed construct file entry: %s\n",
				strerror(fError));
			return fError;
		}

		if (entry.Exists() && entry.IsFile()) {
			// an incomplete file already exists, we can continue the
			// download
			// NOTE: we know this is an imcomplete download since it
			// has the version appended to the server id, and the version
			// is the same as we are trying to download
			if (file.SetTo(&entry, B_READ_WRITE) == B_OK) {
				// read the already existing file so that
				// we are at it's end and have computed
				// the first part of the *real* sha-256 value
				skipData = 0;
				ssize_t read = file.Read(buffer, BUFFER_SIZE);
				while (read > 0) {
					realSHA256.Update(buffer, read);
					skipData += read;
					read = file.Read(buffer, BUFFER_SIZE);
				}
			}
			// read a possibly attached sha-256 attribute
			BNode node(&entry);
			node.ReadAttrString(kSHA256Attr, &existingFileSha256String);
		} else {
			// read a possibly attached sha-256 attribute
			// from the file we want to download in case it already
			// exists - if the sha-256 is the same as the meta data
			// from the server indicates, we don't need to download
			// anything at all
			filePath.SetTo(fPath.Path(), fServerID.String());
			BEntry finalEntry(filePath.Path());
			if (finalEntry.Exists() && finalEntry.IsFile()) {
				BNode node(&finalEntry);
				node.ReadAttrString(kSHA256Attr, &existingFileSha256String);
			}
		}
	}

	// prepare download request
	BMessage request(REQUEST_DOWNLOAD);
	request.AddString("soid", fServerID);
	request.AddInt32("vrsn", fVersion);
	request.AddBool("send data", fGetData);
	request.AddInt64("skip data", skipData);

	// send request
	BMessage metaDataReply;
	fError = connection->SendRequest(&request, &metaDataReply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nDownloadJob: Failed to send request: %s\n", strerror(fError));
		LOG_ERROR("Execute(): Failed to send request: %s\n", strerror(fError));
		return fError;
	}

	// check reply
	if (metaDataReply.what != REQUEST_UPLOAD) {
		if (const char* errorMessage = CheckErrorReply(&metaDataReply)) {
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: Request failed: %s\n", errorMessage);
			LOG_ERROR("Execute(): Request failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: Unexpected reply from server (0x%lx).\n",
					metaDataReply.what);
			LOG_ERROR("Execute(): Unexpected reply from server (0x%lx).\n",
				metaDataReply.what);
			return fError = B_ERROR;
		}
	}

	const char* name;
	const char* type;
	if (metaDataReply.FindString("name", &name) == B_OK
		&& metaDataReply.FindString("type", &type) == B_OK) {
		statusOutput->PrintInfoMessage(", '%s', type: '%s', "
			"v %ld...", name, type, fVersion);
		LOG_DEBUG("Execute(): Meta data: '%s', type: '%s', v %ld...", name,
			type, fVersion);
	}

	off_t fileSize;
	if (fGetData && metaDataReply.FindInt64("size", &fileSize) != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nDownloadJob: Malformed reply from server (no file size).\n");
		LOG_ERROR("Execute(): Malformed reply from server (no file size).\n");
		return fError = B_ERROR;
	}

	// check expected sha-256
	BString newSha256String;
	if (metaDataReply.FindString("sha-256", &newSha256String) != B_OK)
		newSha256String = "";

	if (newSha256String.Length() > 0
		&& existingFileSha256String == newSha256String) {
		// the sha-256 values are both valid and they match
		// -> only download something in case we need to continue
		if (skipData == 0) {
			skippedDataBecauseSHAMatches = true;
			// The server requested an upload! Cancel it!
			// send an "OK" reply to the server with an error code
			BMessage cancelReply(REQUEST_ERROR_REPLY);
			fError = cancelReply.AddString("error", "SHA-256 values match, "
				"no need to download anything.");
			if (fError == B_OK)
				fError = connection->SendRequest(&cancelReply);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nDownloadJob: Failed to send cancel request: %s\n",
					strerror(fError));
				LOG_ERROR("Execute(): Failed to send cancel request: %s\n",
					strerror(fError));
				return fError;
			}
			// prevent the code below from downloading anything
			fileSize = -1;
		} else {
			// just continue with the download
		}
	} else if (skipData > 0 && newSha256String.Length() > 0
		&& existingFileSha256String.Length() > 0
		&& existingFileSha256String != newSha256String) {
		// valid sha-256 values of an unfinished download,
		// but they don't match!
		statusOutput->PrintErrorMessage(
			"\nDownloadJob: SHA-256 values do not match, removing "
			"unfinished previous download with wrong SHA-256. Emitting "
			"error since the server was already asked to seek the data. "
			"Download will be retried in next iteration.\n");
		// inform the server that we encountered an error,
		// so that it does not want to continue with uploading
		BMessage cancelReply(REQUEST_ERROR_REPLY);
		fError = cancelReply.AddString("error", "Need to remove unfinished download.");
		if (fError == B_OK)
			fError = connection->SendRequest(&cancelReply);
		// entry should in this case still point to the previous
		// unfinished download
		entry.Remove();
		// can't continue, as we already requested to seek into
		// the data stream, we need to download this again
		return fError = B_ERROR;
	}

	if (fileSize >= 0 && fileSize < skipData) {
		// on the server, the skip offset has been truncated to the
		// file size as well
		file.Seek(fileSize, SEEK_SET);
	}

	if (fGetData && fileSize >= 0) {
		const char* versionString;
		if (metaDataReply.FindString("vrsn", &versionString) != B_OK) {
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: Malformed reply from server (no version).\n");
			LOG_ERROR("Execute(): Malformed reply from server (no version).\n");
			return fError = B_ERROR;
		}

		int32 version = atoi(versionString);
		if (fVersion != version) {
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: server is trying to send different version "
				"(requested: %ld, received: %ld).\n", fVersion, version);
			LOG_ERROR("Execute(): server is trying to send different version "
				"(requested: %ld, received: %ld).\n", fVersion, version);
			return fError = B_ERROR;
		}

		if (fError == B_OK) {
			if (skipData == 0) {
				if (fData == NULL) {
					// the file was not yet intialised
					fError = file.SetTo(&ref,
						B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
					// append the sha-256 value to the file
					if (newSha256String.Length() > 0) {
						BNode node(&ref);
						node.WriteAttrString(kSHA256Attr, &newSha256String);
					}
				}
			} else {
				// continue a previous download
				fError = file.InitCheck();
				statusOutput->PrintInfoMessage("cnt");
				LOG_DEBUG("Execute(): continuing previous download\n");
			}
		}

		if (fError == B_OK) {
			// send an OK reply to the server
			BMessage okReply(REQUEST_ERROR_REPLY);
			fError = connection->SendRequest(&okReply);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nDownloadJob: Failed to send request: %s\n", strerror(fError));
				LOG_ERROR("Execute(): Failed to send request: %s\n",
					strerror(fError));
				return fError;
			}
		} else {
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: Failed to open file \"%s/%s\": %s\n", fPath.Path(),
				ref.name, strerror(fError));
			LOG_ERROR("Execute(): Failed to open file \"%s/%s\": %s\n",
				fPath.Path(), ref.name, strerror(fError));
	
			// send an error reply to the server
			BMessage errorReply(REQUEST_ERROR_REPLY);
			BString errorMessage = "Failed to open file or send request: ";
			errorMessage << strerror(fError);
			errorReply.AddString("error", errorMessage.String());
			connection->SendRequest(&errorReply);
	
			return fError;
		}
	
		// download data
		BDataIO& data = (fData != NULL ? *fData : file);
		off_t bytesLeft = fileSize - skipData;
		off_t bytesReceived = 0;
		while (bytesLeft > 0) {
			// receive "send data" request from server
			BMessage reply;
			fError = connection->ReceiveRequest(&reply);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nDownloadJob: Failed to receive \"send data\" request: %s\n",
					strerror(fError));
				LOG_ERROR("Execute(): Failed to receive \"send data\" request: "
					"%s\n", strerror(fError));
				return fError;
			}
	
			// check request
			if (reply.what != REQUEST_SEND_DATA) {
				if (const char* errorMessage = CheckErrorReply(&reply)) {
					statusOutput->PrintErrorMessage(
						"\nDownloadJob: Request failed: %s\n", errorMessage);
					LOG_ERROR("Execute(): Request failed: %s\n", errorMessage);
					return fError = B_ERROR;
				} else {
					statusOutput->PrintErrorMessage(
						"\nDownloadJob: Unexpected reply from server (0x%lx).\n",
							reply.what);
					LOG_ERROR("Execute(): Unexpected reply from server "
						"(0x%lx).\n", reply.what);
					return fError = B_ERROR;
				}
			}
	
			int32 chunkSize;
			if (reply.FindInt32("size", &chunkSize) != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nDownloadJob: Malformed reply from server (no chunk size).\n");
				LOG_ERROR("Execute(): Malformed reply from server (no chunk "
					"size).\n");
				return fError = B_ERROR;
			}
	
			// receive the data
			fError = _WriteChunk(connection, statusOutput, data, chunkSize,
				buffer, BUFFER_SIZE);
			if (fError != B_OK)
				return fError;

			// update the real sha-256 value
			realSHA256.Update(buffer, chunkSize);

			// send ok reply
			BMessage okReply(REQUEST_ERROR_REPLY);
			fError = connection->SendRequest(&okReply);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nDownloadJob: Failed to send OK to server: %s\n",
					strerror(fError));
				LOG_ERROR("Execute(): Failed to send OK to server: %s\n",
					strerror(fError));
				return fError;
			}
	
			bytesLeft -= chunkSize;

			// inform about progress
			// TODO: this should be improved somehow, maybe by
			// extending the StatusOutput API
			bytesReceived += chunkSize;
			if (bytesReceived % (1024 * 1024) == 0)
				statusOutput->PrintInfoMessage(".");
		}

		// finished with downloading
		// check the real sha-256 value against the expected one
		BString realSHA256String;
		realSHA256.GetDigestAsString(realSHA256String);
		if (newSha256String.Length() > 0
			&& realSHA256String != newSha256String) {
			// the data was corrupted on the way to us
			statusOutput->PrintErrorMessage(
				"\nDownloadJob: Detected SHA-256 mismatch - "
				"received corrupted data (expected: %s, received: %s)!\n",
				newSha256String.String(), realSHA256String.String());
			LOG_ERROR("Execute(): Detected SHA-256 mismatch - "
				"received corrupted data(expected: %s, received: %s)!\n",
				newSha256String.String(), realSHA256String.String());
			return fError = B_IO_ERROR;
		}

		// rename the temp file and clobber the original file
		if (fData == NULL) {
			fError = entry.Rename(fServerID.String(), true);
			if (fError < B_OK) {
				LOG_ERROR("Execute(): Failed to rename downloaded file: %s\n",
					strerror(fError));
				return fError;
			}

			ref.set_name(fServerID.String());

			// append the sha-256 value to the file
			if (newSha256String.Length() > 0) {
				BNode node(&ref);
				node.WriteAttrString(kSHA256Attr, &newSha256String);
			}

			// everything went well, so we can cleanup any previous
			// download attempts (NOTE: only do this when we were supposed
			// to download the file data!)
			_CleanUpFailedDownloads(statusOutput);
		}
	}

	// tell the user, everything went fine
	if (skippedDataBecauseSHAMatches)
		statusOutput->PrintInfoMessage("completed (sha-256 match)\n");
	else
		statusOutput->PrintInfoMessage("completed\n");
	LOG_INFO("Execute(): Object downloaded successfully\n");

	// inform our target
	BMessage message(metaDataReply.what);
	if (fMessage) {
		message = *fMessage;
	}
	message.AddMessage("meta data", &metaDataReply);
	if (fGetData && fData == NULL) {
		message.AddRef("refs", &ref);
	}
	InformHandler(&message);

	return fError = B_OK;
}


// _WriteChunk
status_t
DownloadJob::_WriteChunk(RequestConnection* connection,
	StatusOutput* statusOutput, BDataIO& file, int32 chunkSize,
	void* buffer, int32 bufferSize)
{
	int32 bytesLeft = chunkSize;
	while (bytesLeft > 0) {
		// receive data
		int32 toRead = min_c(bytesLeft, bufferSize);
		status_t error = connection->ReceiveRawData(buffer, toRead);
		if (error != B_OK) {
			statusOutput->PrintErrorMessage(
				"DownloadJob: Receiving data failed for file \"%s\": %s\n",
				fServerID.String(), strerror(error));
			LOG_ERROR("_WriteChunk(): Receiving data failed for file \"%s\": "
				"%s\n", fServerID.String(), strerror(error));
			return B_ERROR;
		}

		// write data to file
		ssize_t bytesWritten = file.Write(buffer, toRead);
		if (bytesWritten != toRead) {
			error = (bytesWritten < 0 ? bytesWritten : B_ERROR);

			// notify server
			BMessage errorRequest(REQUEST_ERROR_REPLY);
			errorRequest.AddString("error", "Failed to write to file");
			connection->SendRequest(&errorRequest);

			// notify user
			statusOutput->PrintErrorMessage(
				"DownloadJob: Failed to write to file \"%s\": %s\n",
				fServerID.String(), strerror(error));
			LOG_ERROR("_WriteChunk(): Failed to write to file \"%s\": %s\n",
				fServerID.String(), strerror(error));
			return error;
		}

		bytesLeft -= toRead;
	}

	return B_OK;
}

// _CleanUpFailedDownloads
void
DownloadJob::_CleanUpFailedDownloads(StatusOutput* output)
{
	BDirectory downloadDir(fPath.Path());
	if (downloadDir.InitCheck() < B_OK)
		return;

	char fileName[B_FILE_NAME_LENGTH];
	BEntry entry;
	while (downloadDir.GetNextEntry(&entry) == B_OK) {
		if (entry.GetName(fileName) < B_OK)
			continue;
		if (fServerID == fileName) {
			// the file we just downloaded
			continue;
		}
		BString nameWithVersion(fServerID);
		nameWithVersion << ':';
		if (nameWithVersion.Compare(fileName, nameWithVersion.Length()) == 0) {
			// the filename is the same as the file we were supposed
			// to download, with a version appended, so it is a left-over
			// incomplete download
			output->PrintInfoMessage("\nDownloadJob: deleting incomplete "
				"download: %s\n", fileName);
			LOG_INFO("_CleanUpFailedDownloads(): deleting incomplete download: "
				"%s\n", fileName);
			entry.Remove();
		}
	}
}


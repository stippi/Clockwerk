/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "UploadJob.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "AutoDeleter.h"
#include "RequestConnection.h"
#include "RequestMessageCodes.h"
#include "SHA256.h"

#include "StatusOutput.h"


using std::nothrow;

// constructor
UploadJob::UploadJob(const entry_ref& ref, const BMessage* metaData,
					 BHandler* handler, BMessage* message)
	: InformingJob(handler, message),
	  fRef(ref),
	  fData(NULL),
	  fMetaData(metaData ? *metaData : BMessage())
{
}

// constructor
UploadJob::UploadJob(BPositionIO* data, const BMessage* metaData,
					 BHandler* handler, BMessage* message)
	: InformingJob(handler, message),
	  fRef(),
	  fData(data),
	  fMetaData(metaData ? *metaData : BMessage())
{
}

// destructor
UploadJob::~UploadJob()
{
	delete fData;
}

// Execute
status_t
UploadJob::Execute(JobConnection* jobConnection)
{
	fError = B_OK;
	// open the file
	if (fRef.name) {
		BFile* file = new (nothrow) BFile(&fRef, B_READ_ONLY);
		if (!file) {
			fError = B_NO_MEMORY;
		} else {
			fData = file;
			fError = file->InitCheck();
		}
	}
	if (fError != B_OK)
		return fError;

	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection)
		return fError = ENOTCONN;

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();
	const char* fileName;
	if (fMetaData.FindString("name", &fileName) < B_OK)
		fileName = fRef.name;
	const char* type;
	if (fMetaData.FindString("type", &type) < B_OK)
		type = "unkown";
	const char* currentVersion;
	if (fMetaData.FindString("vrsn", &currentVersion) < B_OK)
		currentVersion = "??";

	// find object id
	BString id;
	if (fMetaData.FindString("soid", &id) < B_OK) {
		return fError = B_BAD_VALUE;
	}

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"UploadJob: '%s' (type '%s', id '%s', version %s)...",
			fileName, type, id.String(), currentVersion);

	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nUploadJob: Failed to open file '%s': %s\n", fileName,
			strerror(fError));
		return fError;
	}

	off_t size = -1;
	SHA256 sha256;

	if (fData) {
		// compute the SHA256 digest of the file data

		// NOTE: this also works arround a weird BFS problem,
		// obtaining the file size by reading the entire file
		size = 0;
		size_t chunkSize = 16 * 1024;
		uint8 buffer[chunkSize];
		fError = (status_t)fData->Seek(0, SEEK_SET);
		ssize_t read = fData->Read(buffer, chunkSize);
		while (read > 0) {
			sha256.Update(buffer, read);
			size += read;
			read = fData->Read(buffer, chunkSize);
		}

		if (size >= 0)
			fError = (status_t)fData->Seek(0, SEEK_SET);
		else
			fError = size;

		if (fError != B_OK) {
			statusOutput->PrintErrorMessage(
				"\nUploadJob: Failed to get size of file '%s': %s\n",
				fileName, strerror(fError));
			return fError;
		}
	}

//printf("uploading meta data:\n");
//fMetaData.PrintToStream();
//
	// prepare upload request
	BMessage request(fMetaData);
	request.what = REQUEST_UPLOAD;
	request.AddInt64("size", size);
	if (fData) {
		BString sha256String;
		sha256.GetDigestAsString(sha256String);
		request.AddString("sha-256", sha256String.String());
	}

	// send request
	BMessage reply;
	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nUploadJob: Failed to send request: %s\n", strerror(fError));
		return fError;
	}

	// check error
	if (connection->ProtocolVersion() < 3 || fData) {
		// Since protocol version 3 we don't get an OK anymore for data-less
		// objects.
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage("\nUploadJob: Upload failed: %s\n",
				errorMessage);
			return fError = B_ERROR;
		}
	}

	if (fData) {
		// allocate buffer
		enum {
			BUFFER_SIZE	= 1024 * 1024
		};
		void* buffer = malloc(BUFFER_SIZE);
		if (!buffer) {
			statusOutput->PrintErrorMessage("\nUploadJob: Out of memory\n");
			return fError = B_NO_MEMORY;
		}
		MemoryDeleter _(buffer);
	
		// upload data
		off_t bytesLeft = size;
		off_t bytesSent = 0;
		while (bytesLeft > 0) {
			// read from file
			int32 toRead = (int32)min_c(bytesLeft, BUFFER_SIZE);
			ssize_t bytesRead = fData->Read(buffer, toRead);
			if (bytesRead != toRead) {
				fError = (bytesRead < 0 ? bytesRead : B_ERROR);
	
				// notify server
				BMessage errorRequest(REQUEST_ERROR_REPLY);
				errorRequest.AddString("error", "Failed to read from file");
				connection->SendRequest(&errorRequest);
	
				// notify user
				statusOutput->PrintErrorMessage(
					"\nUploadJob: Failed to read from file '%s': %s\n",
					fileName, strerror(fError));
				return fError;
			}
	
			// send "send data" request
			BMessage sendDataRequest(REQUEST_SEND_DATA);
			sendDataRequest.AddInt32("size", toRead);
			fError = connection->SendRequest(&sendDataRequest);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nUploadJob: Failed to send data from file '%s': %s\n",
					fileName, strerror(fError));
			}
	
			// send data
			fError = connection->SendRawData(buffer, toRead);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nUploadJob: Failed to send data from file '%s': %s\n",
					fileName, strerror(fError));
				return fError;
			}
	
			// receive the OK request
			fError = connection->ReceiveRequest(&reply);
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nUploadJob: Failed to receive \"ok\" request: %s\n",
					strerror(fError));
				return fError;
			}
	
			if (const char* errorMessage = CheckErrorReply(&reply)) {
				statusOutput->PrintErrorMessage(
					"\nUploadJob: Sending data failed for file '%s': %s\n",
					fileName, errorMessage);
				return fError = B_ERROR;
			}
	
			bytesLeft -= toRead;

			// inform about progress
			// TODO: this should be improved somehow, maybe by
			// extending the StatusOutput API
			bytesSent += toRead;
			if (bytesSent % (1024 * 1024) == 0)
				statusOutput->PrintInfoMessage(".");
		}
	}

	// receive the final "Upload OK" request
	if (connection->ProtocolVersion() < 3 || fData) {
		// Since protocol version 3 only for non data-less objects -- otherwise
		// we already got the reply.
		fError = connection->ReceiveRequest(&reply);
	}

	// check reply
	if (reply.what != REQUEST_UPLOAD_OK_REPLY) {
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage(
				"\nUploadJob: Upload failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage(
				"\nUploadJob: Unexpected reply from server (0x%lx).\n",
					reply.what);
			return fError = B_ERROR;
		}
	}

	int32 version = 0;
	if (reply.FindInt32("vrsn", &version) < B_OK) {
		statusOutput->PrintErrorMessage("\nUploadJob: No version in "
			"reply!\n");
		return fError = B_ERROR;
	}

	// inform the application about the successful upload
	BMessage message(reply.what);
	if (fMessage) {
		message = *fMessage;
	}
	message.AddString("soid", id);
	message.AddInt32("version", version);
	InformHandler(&message);

	// tell the user, everything went fine		
	statusOutput->PrintInfoMessage(
		"completed. Server assigned version: %ld\n", version);

	return fError = B_OK;
}

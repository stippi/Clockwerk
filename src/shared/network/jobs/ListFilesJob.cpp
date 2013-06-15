/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ListFilesJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "Logger.h"
#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"


static Logger sLog("network.jobs.ListFilesJob");


static const int32 kMaxListingSize = 1000;


// constructor
ListFilesJob::ListFilesJob(const BString& clientID, int32 maxCount,
		BHandler* target, BMessage* message)
	: InformingJob(target, message),
	  fClientID(clientID),
	  fMaxCount(maxCount)
{
}

// constructor
ListFilesJob::ListFilesJob(const BString& clientID,
		BHandler* target, BMessage* message)
	: InformingJob(target, message),
	  fClientID(clientID),
	  fMaxCount(-1)
{
}

// constructor
ListFilesJob::ListFilesJob(BHandler* target, BMessage* message)
	: InformingJob(target, message),
	  fMaxCount(-1)
{
}

// destructor
ListFilesJob::~ListFilesJob()
{
}

// Execute
status_t
ListFilesJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection) {
		return fError = ENOTCONN;
	}

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"ListFilesJob: Receiving object listing..");

	BMessage listing;
	BString queryCookie;
	int32 startIndex = 0;
	int32 maxListingSize = (fMaxCount >= 0 ? fMaxCount : kMaxListingSize);

	while (true) {
		statusOutput->PrintInfoMessage(".");

		// prepare the request
		BMessage request(REQUEST_LIST_FILES);
		if (fClientID != "") {
			fError = request.AddString("scop", fClientID.String());
			if (fError != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nListFilesJob: Failed to add scope to message: %s\n",
					strerror(fError));
				LOG_ERROR("Execute(): Failed to add scope to message: %s\n",
					strerror(fError));
				return fError;
			}
		}

		// add result set restrictions
		if (connection->ProtocolVersion() >= 3) {
			if (queryCookie.Length() > 0)
				fError = request.AddString("results cookie", queryCookie);
			if (fError == B_OK)			
				fError = request.AddInt32("results start index", startIndex);
			if (fError == B_OK)			
				fError = request.AddInt32("results max count", maxListingSize);

			if (fError != B_OK) {
				statusOutput->PrintErrorMessage("\nListFilesJob: Failed to "
					"add result set restrictions to message: %s\n",
					strerror(fError));
				LOG_ERROR("Execute(): Failed to "
					"add result set restrictions to message: %s\n",
					strerror(fError));
				return fError;
			}
		}

		// send request
		BMessage reply;

		fError = connection->SendRequest(&request, &reply);
		if (fError != B_OK) {
			statusOutput->PrintErrorMessage(
				"\nListFilesJob: Failed to send request: %s\n",
				strerror(fError));
			LOG_ERROR("Execute(): Failed to send request: %s\n",
				strerror(fError));
			return fError;
		}
		fError = B_ERROR;

		// check error
		if (reply.what != REQUEST_LIST_FILES_REPLY) {
			if (const char* errorMessage = CheckErrorReply(&reply)) {
				statusOutput->PrintErrorMessage(
					"\nListFilesJob: Request failed: %s\n", errorMessage);
				LOG_ERROR("Execute(): Request failed: %s\n", errorMessage);
				return fError = B_ERROR;
			} else {
				statusOutput->PrintErrorMessage(
					"\nListFilesJob: Unexpected reply from server (0x%lx or "
					"%.4s).\n", reply.what, (char*)&reply.what);
				LOG_ERROR("Execute(): Unexpected reply from server "
					"(0x%lx or %.4s).\n", reply.what, (char*)&reply.what);
				return fError = B_ERROR;
			}
		}

//		// print the files
//		const char* fileName;
//		for (int32 i = 0;
//			 reply.FindString("name", i, &fileName) == B_OK;
//			 i++) {
//			int64 fileSize;
//			int32 version;
//			const char* serverID;
//			if (reply.FindInt64("size", i, &fileSize) == B_OK
//				&& reply.FindInt32("vrsn", i, &version) == B_OK
//				&& reply.FindString("soid", i, &serverID) == B_OK) {
//				statusOutput->PrintInfoMessage(
//					"  server file: \"%s\", id: %s, size: %lld, version: %ld\n",
//					fileName, serverID, fileSize, version);
//			} else {
//				statusOutput->PrintInfoMessage(
//					"  server file: \"%s\", no size and/or version given!\n", fileName);
//			}
//		}

		// we're done for older protocol versions
		if (connection->ProtocolVersion() < 3) {
			listing = reply;
			break;
		}

		// check well-formedness of the reply
		int32 count;
		type_code type;
        if (reply.GetInfo("name", &type, &count) != B_OK) {
        	// that means we're probably done
        	break;
        }

		int32 otherCount;
		if (type != B_STRING_TYPE
			|| reply.GetInfo("size", &type, &otherCount) != B_OK
			|| type != B_INT64_TYPE
			|| otherCount != count
			|| reply.GetInfo("type", &type, &otherCount) != B_OK
			|| type != B_STRING_TYPE
			|| otherCount != count
			|| reply.GetInfo("soid", &type, &otherCount) != B_OK
			|| type != B_STRING_TYPE
			|| otherCount != count
			|| reply.GetInfo("vrsn", &type, &otherCount) != B_OK
			|| type != B_INT32_TYPE
			|| otherCount != count
			|| reply.FindString("results cookie", &queryCookie) != B_OK) {
			statusOutput->PrintErrorMessage(
				"\nListFilesJob: Malformed reply\n");
			LOG_ERROR("Execute(): Malformed reply\n");
			return fError = B_ERROR;
		}

		// copy the reply fields to the listing message
		for (int32 k = 0; k < count; k++) {
			const char* name;
			const char* type;
			const char* id;
			int64 size;
			int32 version;
			
	        if (reply.FindString("name", k, &name) != B_OK
				|| reply.FindInt64("size", k, &size) != B_OK
				|| reply.FindString("type", k, &type) != B_OK
				|| reply.FindString("soid", k, &id) != B_OK
				|| reply.FindInt32("vrsn", k, &version) != B_OK
				|| listing.AddString("name", name) != B_OK
				|| listing.AddInt64("size", size) != B_OK
				|| listing.AddString("type", type) != B_OK
				|| listing.AddString("soid", id) != B_OK
				|| listing.AddInt32("vrsn", version) != B_OK) {
				statusOutput->PrintErrorMessage(
					"\nListFilesJob: Failed to add entry to listing\n");
				LOG_ERROR("Execute(): Failed to add entry to listing\n");
				return fError = B_ERROR;
	        }
		}

		startIndex += count;
		if (fMaxCount >= 0 && startIndex >= fMaxCount)
			break;
	}

	// tell the user, that we're done
	statusOutput->PrintInfoMessage("done (%ld objects on server)\n", startIndex);

	// inform our target
#ifndef LIST_FILES_JOB_REPLY_IS_LISTING
	BMessage message(listing.what);
	if (fMessage) {
		message = *fMessage;
	}
	fError = message.AddMessage("listing", &listing);
	if (fError < B_OK) {
		statusOutput->PrintErrorMessage(
			"\nListFilesJob: failed adding server listing to BMessage: %s\n",
			strerror(fError));
		LOG_ERROR("Execute(): failed adding server listing to BMessage: "
			"%s.\n", strerror(fError));
		return fError;
	}
	InformHandler(&message);
#else
	if (fMessage) {
		// copy our message into listing
		listing.what = fMessage->what;
#ifdef CLOCKWERK_FOR_ZETA
		const
#endif
		char* name;

		uint32 type;
		int32 count;
		for (int32 i = 0;
			fMessage->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
			i++) {
			for (int32 k = 0; k < count; k++) {
				const void* data;
				ssize_t numBytes;
				if (fMessage->FindData(name, type, &data, &numBytes) < B_OK
					|| listing.AddData(name, type, data, numBytes) < B_OK)
					break;
			}
		}
	}
	InformHandler(&listing);
#endif
	return fError = B_OK;
}

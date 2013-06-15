/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "QueryFilesJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"


// constructor
QueryFilesJob::QueryFilesJob(BHandler* target, BMessage* message)
	: InformingJob(target, message),
	  fRequest(REQUEST_LIST_FILES)
{
}


// destructor
QueryFilesJob::~QueryFilesJob()
{
}


// SetPredicate
status_t
QueryFilesJob::SetPredicate(const char* name, type_code type, const void* data,
	ssize_t numBytes)
{
	fRequest.RemoveName(name);
	if (data == NULL)
		return B_OK;

	return fRequest.AddData(name, type, data, numBytes);
}


// SetStringPredicate
status_t
QueryFilesJob::SetStringPredicate(const char* name, const char* data)
{
	return SetPredicate(name, B_STRING_TYPE, data,
		(data ? strlen(data) + 1 : 0));
}


// SetScopePredicate
status_t
QueryFilesJob::SetScopePredicate(const char* scope)
{
	return SetStringPredicate("scop", scope);
}


// SetNamePredicate
status_t
QueryFilesJob::SetNamePredicate(const char* name)
{
	return SetStringPredicate("name", name);
}


// SetIDPredicate
status_t
QueryFilesJob::SetIDPredicate(const char* objectID)
{
	return SetStringPredicate("soid", objectID);
}


// Execute
status_t
QueryFilesJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection) {
		return fError = ENOTCONN;
	}

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"QueryFilesJob: Starting listing of server files...\n");

	// send request
	BMessage reply;

	fError = connection->SendRequest(&fRequest, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"QueryFilesJob: Failed to send request: %s\n", strerror(fError));
		return fError;
	}
	fError = B_ERROR;

	// check error
	if (reply.what != REQUEST_LIST_FILES_REPLY) {
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage(
				"QueryFilesJob: Request failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage("QueryFilesJob: Unexpected reply "
				"from server (0x%lx or %.4s).\n", reply.what,
				(char*)&reply.what);
			return fError = B_ERROR;
		}
	}

//	// print the files
//	const char* fileName;
//	for (int32 i = 0;
//		 reply.FindString("name", i, &fileName) == B_OK;
//		 i++) {
//		int64 fileSize;
//		int32 version;
//		const char* serverID;
//		if (reply.FindInt64("size", i, &fileSize) == B_OK
//			&& reply.FindInt32("vrsn", i, &version) == B_OK
//			&& reply.FindString("soid", i, &serverID) == B_OK) {
//			statusOutput->PrintInfoMessage(
//				"  server file: \"%s\", id: %s, size: %lld, version: %ld\n",
//				fileName, serverID, fileSize, version);
//		} else {
//			statusOutput->PrintInfoMessage(
//				"  server file: \"%s\", no size and/or version given!\n", fileName);
//		}
//	}

	// inform our target
	BMessage message(reply.what);
	if (fMessage) {
		message = *fMessage;
	}
	message.AddMessage("listing", &reply);
	InformHandler(&message);

	// tell the user, that we're done
	statusOutput->PrintInfoMessage(
		"QueryFilesJob: Listing done\n");

	return fError = B_OK;
}

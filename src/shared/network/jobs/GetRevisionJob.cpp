/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */


#include "GetRevisionJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"

GetRevisionJob::GetRevisionJob(int64 clientRevision, BHandler* target, BMessage* message)
	: InformingJob(target, message),
	fClientRevision(clientRevision)
{
}


GetRevisionJob::~GetRevisionJob()
{
}


status_t
GetRevisionJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (connection == NULL)
		return fError = ENOTCONN;

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
//	statusOutput->PrintInfoMessage(
//		"GetRevisionJob: Starting getting revision...");

	// prepare the request
	BMessage request(REQUEST_GET_REVISION);
	fError = request.AddInt64("revision number", fClientRevision);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nGetRevisionJob: Failed to add revision number to message: %s\n",
			strerror(fError));
		return fError;
	}

	// send request
	BMessage reply;

	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nGetRevisionJob: Failed to send request: %s\n", strerror(fError));
		return fError;
	}
	fError = B_ERROR;

	// check error
	if (reply.what != REQUEST_GET_REVISION_REPLY) {
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage(
				"\nGetRevisionJob: Request failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage(
				"\nGetRevisionJob: Unexpected reply from server (0x%lx or %.4s).\n",
					reply.what, (char*)&reply.what);
			return fError = B_ERROR;
		}
	}

//	reply.PrintToStream();

	// inform our target
	BMessage message(reply.what);
	if (fMessage) {
		message = *fMessage;
	}
	message.AddMessage("revision", &reply);
	InformHandler(&message);

	// tell the user, that we're done
//	statusOutput->PrintInfoMessage("done\n");

	return fError = B_OK;
}

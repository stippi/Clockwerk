/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "StartTransactionJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"

StartTransactionJob::StartTransactionJob(const char* changeSetID,
		const char* client, const char* user, BHandler* target,
		BMessage* message)
	: InformingJob(target, message),
	fChangeSetID(changeSetID),
	fClient(client),
	fUser(user)
{
}


StartTransactionJob::~StartTransactionJob()
{
}


status_t
StartTransactionJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (connection == NULL)
		return fError = ENOTCONN;

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"StartTransactionJob: Starting transaction '%s'...",
		fChangeSetID.String());

	// prepare the request
	BMessage request(REQUEST_START_TRANSACTION);
	fError = request.AddString("change set id", fChangeSetID);
	if (fError == B_OK)
		fError = request.AddString("client", fClient);
	if (fError == B_OK)
		fError = request.AddString("user", fUser);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nStartTransactionJob: Failed to build the request message: %s\n",
			strerror(fError));
		return fError;
	}

	// send request
	BMessage reply;

	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nStartTransactionJob: Failed to send request: %s\n",
			strerror(fError));
		return fError;
	}
	fError = B_ERROR;

	// check error
	if (reply.what != REQUEST_ERROR_REPLY) {
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage(
				"\nStartTransactionJob: Request failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage(
				"\nStartTransactionJob: Unexpected reply from server "
				"(0x%lx or %.4s).\n", reply.what, (char*)&reply.what);
			return fError = B_ERROR;
		}
	}

//	reply.PrintToStream();

	// inform our target
	BMessage message(reply.what);
	if (fMessage) {
		message = *fMessage;
	}
	message.AddMessage("reply", &reply);
	InformHandler(&message);

	// tell the user, that we're done
	statusOutput->PrintInfoMessage("done\n");

	return fError = B_OK;
}

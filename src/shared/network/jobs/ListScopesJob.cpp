/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ListScopesJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"

// constructor
ListScopesJob::ListScopesJob(BHandler* target, BMessage* message)
	: InformingJob(target, message)
{
}

// destructor
ListScopesJob::~ListScopesJob()
{
}

// Execute
status_t
ListScopesJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection) {
		return fError = ENOTCONN;
	}

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"ListScopesJob: Starting listing of server files...\n");

	// prepare the request
	BMessage request(REQUEST_LIST_SCOPES);

	// send request
	BMessage reply;

	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"ListScopesJob: Failed to send request: %s\n", strerror(fError));
		return fError;
	}
	fError = B_ERROR;

	// check error
	if (reply.what != REQUEST_LIST_SCOPES_REPLY) {
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage(
				"ListScopesJob: Request failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage(
				"ListScopesJob: Unexpected reply from server (0x%lx or %.4s).\n",
					reply.what, (char*)&reply.what);
			return fError = B_ERROR;
		}
	}

	// print the scopes
	const char* scope;
	for (int32 i = 0;
		 reply.FindString("scop", i, &scope) == B_OK;
		 i++) {
		statusOutput->PrintInfoMessage(
			"  server scope: \"%s\"\n", scope);
	}

	// inform our target
	BMessage message(reply.what);
	if (fMessage)
		message = *fMessage;

	message.AddMessage("listing", &reply);
	InformHandler(&message);

	// tell the user, that we're done
	statusOutput->PrintInfoMessage(
		"ListScopesJob: Listing done\n");

	return fError = B_OK;
}

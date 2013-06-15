/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SyncWithSuperiorJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"

SyncWithSuperiorJob::SyncWithSuperiorJob(BHandler* target,
		BMessage* message)
	: InformingJob(target, message)
{
}


SyncWithSuperiorJob::~SyncWithSuperiorJob()
{
}


status_t
SyncWithSuperiorJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (connection == NULL)
		return fError = ENOTCONN;

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"SyncWithSuperiorJob: Telling server to sync with superior...");

	// send request
	BMessage request(REQUEST_SYNC_WITH_SUPERIOR);
	BMessage reply;

	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nSyncWithSuperiorJob: Failed to send request: %s\n",
			strerror(fError));
		return fError;
	}
	fError = B_ERROR;

	// check error
	if (const char* errorMessage = CheckErrorReply(&reply)) {
		statusOutput->PrintErrorMessage("\nSyncWithSuperiorJob: failed: %s\n",
			errorMessage);
		return fError = B_ERROR;
	}

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

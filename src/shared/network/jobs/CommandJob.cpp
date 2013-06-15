/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CommandJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>

#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"

// constructor
CommandJob::CommandJob(const BString& clientID, uint32 command,
		const char* commandName, BHandler* target, BMessage* message)
	: InformingJob(target, message)
	, fCommand(command)
	, fCommandName(commandName)
{
}

// destructor
CommandJob::~CommandJob()
{
}

// Execute
status_t
CommandJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection) {
		return fError = ENOTCONN;
	}

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();

	// tell the user, that we're starting
	statusOutput->PrintInfoMessage(
		"CommandJob: Sending '%s' command...\n", fCommandName.String());

	// prepare the request
	BMessage request(fCommand);

	// send request
	BMessage reply;

	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"CommandJob: Failed to send request: %s\n", strerror(fError));
		return fError;
	}
	fError = B_ERROR;

	// check error
	if (reply.what != REQUEST_ERROR_REPLY) {
		if (const char* errorMessage = CheckErrorReply(&reply)) {
			statusOutput->PrintErrorMessage(
				"CommandJob: Request failed: %s\n", errorMessage);
			return fError = B_ERROR;
		} else {
			statusOutput->PrintErrorMessage(
				"CommandJob: Unexpected reply from server (0x%lx or %.4s).\n",
					reply.what, (char*)&reply.what);
			return fError = B_ERROR;
		}
	}

	// inform our target
	BMessage message(reply.what);
	if (fMessage)
		message = *fMessage;

	message.AddMessage("reply", &reply);
	InformHandler(&message);

	// tell the user, that we're done
	statusOutput->PrintInfoMessage(
		"CommandJob: '%s' command successful\n", fCommandName.String());

	return fError = B_OK;
}

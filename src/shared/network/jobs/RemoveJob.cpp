/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RemoveJob.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "AutoDeleter.h"
#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"


using std::nothrow;

// constructor
RemoveJob::RemoveJob(const BMessage* metaData, BHandler* handler,
		BMessage* message)
	: InformingJob(handler, message),
	  fMetaData(metaData ? *metaData : BMessage())
{
}

// destructor
RemoveJob::~RemoveJob()
{
}

// Execute
status_t
RemoveJob::Execute(JobConnection* jobConnection)
{
	RequestConnection* connection = jobConnection->GetRequestConnection();
	if (!connection)
		return fError = ENOTCONN;

	StatusOutput* statusOutput = jobConnection->GetStatusOutput();
	const char* fileName;
	if (fMetaData.FindString("name", &fileName) < B_OK)
		fileName = "unkown";
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
		"RemoveJob: '%s' (type '%s', id '%s', version %s)...",
			fileName, type, id.String(), currentVersion);

	// prepare remove request
	BMessage request;
	request.what = REQUEST_REMOVE;
	fError = request.AddString("soid", id.String());
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nRemoveJob: Failed to prepare request: %s\n", strerror(fError));
		return fError;
	}

	// send request
	BMessage reply;
	fError = connection->SendRequest(&request, &reply);
	if (fError != B_OK) {
		statusOutput->PrintErrorMessage(
			"\nRemoveJob: Failed to send request: %s\n", strerror(fError));
		return fError;
	}

	// check error
	if (const char* errorMessage = CheckErrorReply(&reply)) {
		statusOutput->PrintErrorMessage("\nRemoveJob: Remove failed: %s\n",
			errorMessage);
		return fError = B_ERROR;
	}

	// inform the application about the successful upload
	BMessage message(reply.what);
	if (fMessage) {
		message = *fMessage;
	} else {
		message.AddString("soid", id);
	}

	InformHandler(&message);

	// tell the user, everything went fine		
	statusOutput->PrintInfoMessage("completed.\n");

	return fError = B_OK;
}

/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InformingJob.h"

#include <stdio.h>
#include <string.h>

#include <Looper.h>
#include <Message.h>


InformingJobCallBack::~InformingJobCallBack()
{
}


// constructor
InformingJob::InformingJob(BHandler* handler, BMessage* message)
	: fHandler(handler),
	  fMessage(message),
	  fError(B_OK),
	  fCallBack(NULL)
{
}

// destructor
InformingJob::~InformingJob()
{
	if (fError < B_OK && (fHandler || fCallBack) && fMessage) {
		// we didn't send any message
		// inform our target of the error
		BMessage message(*fMessage);
		message.AddInt32("error", fError);
		InformHandler(&message);
	}
	delete fMessage;
}

// InformHandler
status_t
InformingJob::InformHandler(BMessage* message)
{
	status_t ret = B_NO_INIT;

	if (fCallBack) {
		ret = fCallBack->HandleResult(this, message);
		if (ret < B_OK)
			return ret;
	}

	if (!fHandler)
		return ret;

	// inform our target
	if (BLooper* looper = fHandler->Looper()) {
		if (message)
			return looper->PostMessage(message, fHandler);
		else
			return looper->PostMessage(fMessage, fHandler);
	}

	return ret;
}

// SetMessage
void
InformingJob::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
}

// SetCallBack
void
InformingJob::SetCallBack(InformingJobCallBack* callBack)
{
	fCallBack = callBack;
}


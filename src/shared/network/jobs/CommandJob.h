/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LIST_SCOPES_JOB_H
#define LIST_SCOPES_JOB_H

#include <String.h>
#include "InformingJob.h"

class CommandJob : public InformingJob {
public:
								CommandJob(const BString& clientID,
									uint32 command,
									const char* commandName, BHandler* target,
									BMessage* message);
	virtual						~CommandJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

private:
			BString				fClientID;
			uint32				fCommand;
			BString				fCommandName;
};

#endif	// LIST_SCOPES_JOB_H

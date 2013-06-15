/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef GET_CHANGE_SET_JOB_H
#define GET_CHANGE_SET_JOB_H

#include "InformingJob.h"

#include <String.h>

class GetChangeSetJob : public InformingJob {
public:
								GetChangeSetJob(const char* changeSetID,
									BHandler* target, BMessage* message);
	virtual						~GetChangeSetJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

private:
			BString				fChangeSetID;
};

#endif	// GET_CHANGE_SET_JOB_H

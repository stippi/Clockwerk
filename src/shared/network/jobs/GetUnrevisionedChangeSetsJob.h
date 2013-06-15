/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef GET_UNREVISIONED_CHANGE_SETS_JOB_H
#define GET_UNREVISIONED_CHANGE_SETS_JOB_H

#include "InformingJob.h"

#include <String.h>

class GetUnrevisionedChangeSetsJob : public InformingJob {
public:
								GetUnrevisionedChangeSetsJob(BHandler* target,
									BMessage* message);
	virtual						~GetUnrevisionedChangeSetsJob();

	virtual	status_t			Execute(JobConnection* jobConnection);
};

#endif	// GET_UNREVISIONED_CHANGE_SETS_JOB_H

/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SYNC_WITH_SUPERIOR_JOB_H
#define SYNC_WITH_SUPERIOR_JOB_H

#include "InformingJob.h"

#include <String.h>

class SyncWithSuperiorJob : public InformingJob {
public:
								SyncWithSuperiorJob(BHandler* target,
									BMessage* message);
	virtual						~SyncWithSuperiorJob();

	virtual	status_t			Execute(JobConnection* jobConnection);
};

#endif	// SYNC_WITH_SUPERIOR_JOB_H

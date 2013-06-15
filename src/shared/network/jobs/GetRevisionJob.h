/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef GET_REVISION_JOB_H
#define GET_REVISION_JOB_H

#include "InformingJob.h"

#include <String.h>

class GetRevisionJob : public InformingJob {
public:
								GetRevisionJob(int64 clientRevision, BHandler* target,
									BMessage* message);
	virtual						~GetRevisionJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

private:
			int64				fClientRevision;
};

#endif	// GET_REVISION_JOB_H

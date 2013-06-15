/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LIST_SCOPES_JOB_H
#define LIST_SCOPES_JOB_H

#include <String.h>
#include "InformingJob.h"

class ListScopesJob : public InformingJob {
public:
								ListScopesJob(BHandler* target,
											  BMessage* message);
	virtual						~ListScopesJob();

	virtual	status_t			Execute(JobConnection* jobConnection);
};

#endif	// LIST_SCOPES_JOB_H

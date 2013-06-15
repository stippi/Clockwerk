/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLEAR_STORES_JOB_H
#define CLEAR_STORES_JOB_H

#include "InformingJob.h"

#include <String.h>

class ClearStoresJob : public InformingJob {
public:
								ClearStoresJob(BHandler* target,
									BMessage* message);
	virtual						~ClearStoresJob();

	virtual	status_t			Execute(JobConnection* jobConnection);
};

#endif	// CLEAR_STORES_JOB_H

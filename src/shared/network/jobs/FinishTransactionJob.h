/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef FINISH_TRANSACTION_JOB_H
#define FINISH_TRANSACTION_JOB_H

#include "InformingJob.h"

#include <String.h>

class FinishTransactionJob : public InformingJob {
public:
								FinishTransactionJob(BHandler* target,
									BMessage* message);
	virtual						~FinishTransactionJob();

	virtual	status_t			Execute(JobConnection* jobConnection);
};

#endif	// FINISH_TRANSACTION_JOB_H

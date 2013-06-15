/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef START_TRANSACTION_JOB_H
#define START_TRANSACTION_JOB_H

#include "InformingJob.h"

#include <String.h>

class StartTransactionJob : public InformingJob {
public:
								StartTransactionJob(const char* changeSetID,
									const char* client, const char* user,
									BHandler* target,
									BMessage* message);
	virtual						~StartTransactionJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

private:
			BString				fChangeSetID;
			BString				fClient;
			BString				fUser;
};

#endif	// START_TRANSACTION_JOB_H

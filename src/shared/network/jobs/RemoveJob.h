/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REMOVE_JOB_H
#define REMOVE_JOB_H

#include <Entry.h>
#include <File.h>
#include <Message.h>

#include "InformingJob.h"

class RemoveJob : public InformingJob {
public:
								RemoveJob(const BMessage* metaData,
									BHandler* handler,
									BMessage* message);

	virtual						~RemoveJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

private:
			BMessage			fMetaData;
};

#endif	// REMOVE_JOB_H

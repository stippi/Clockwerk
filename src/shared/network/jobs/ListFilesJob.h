/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LIST_FILES_JOB_H
#define LIST_FILES_JOB_H

#include <String.h>
#include "InformingJob.h"

#define LIST_FILES_JOB_REPLY_IS_LISTING 1

class ListFilesJob : public InformingJob {
public:
								ListFilesJob(const BString& clientID,
									int32 maxCount,
									BHandler* target,
									BMessage* message);
								ListFilesJob(const BString& clientID,
									BHandler* target,
									BMessage* message);
								ListFilesJob(BHandler* target,
									BMessage* message);
	virtual						~ListFilesJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

 private:
			BString				fClientID;
			int32				fMaxCount;
};

#endif	// LIST_FILES_JOB_H

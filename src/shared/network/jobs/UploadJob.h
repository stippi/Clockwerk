/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef UPLOAD_JOB_H
#define UPLOAD_JOB_H

#include <Entry.h>
#include <File.h>
#include <Message.h>

#include "InformingJob.h"

class UploadJob : public InformingJob {
public:
								UploadJob(const entry_ref& ref,
									const BMessage* metaData,
									BHandler* handler,
									BMessage* message);

								UploadJob(BPositionIO* data,
									const BMessage* metaData,
									BHandler* handler,
									BMessage* message);

	virtual						~UploadJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

private:
			entry_ref			fRef;
			BPositionIO*		fData;
			BMessage			fMetaData;
};

#endif	// UPLOAD_JOB_H

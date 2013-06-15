/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef QUERY_FILES_JOB_H
#define QUERY_FILES_JOB_H

#include <Message.h>
#include <String.h>

#include "InformingJob.h"


class QueryFilesJob : public InformingJob {
public:
								QueryFilesJob(BHandler* target,
									BMessage* message);
	virtual						~QueryFilesJob();

			status_t			SetPredicate(const char* name, type_code type,
									const void* data, ssize_t numBytes);
			status_t			SetStringPredicate(const char* name,
									const char* data);

			status_t			SetScopePredicate(const char* scope);
			status_t			SetNamePredicate(const char* name);
			status_t			SetIDPredicate(const char* objectID);

	virtual	status_t			Execute(JobConnection* jobConnection);

 private:
			BMessage			fRequest;
};

#endif	// QUERY_FILES_JOB_H

/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INFORMING_JOB_H
#define INFORMING_JOB_H

#include "JobConnection.h"


class BHandler;
class BMessage;
class InformingJob;

class InformingJobCallBack {
 public:
	virtual						~InformingJobCallBack();

	virtual	status_t			HandleResult(InformingJob* job,
											 const BMessage* result) = 0;
};


class InformingJob : public Job {
public:
								InformingJob(BHandler* handler,
									BMessage* message);
	virtual						~InformingJob();

			void				SetMessage(BMessage* message);
			void				SetCallBack(InformingJobCallBack* callBack);
protected:
			status_t			InformHandler(BMessage* message);

			BHandler*			fHandler;
			BMessage*			fMessage;
			status_t			fError;

			InformingJobCallBack* fCallBack;
};

#endif	// INFORMING_JOB_H

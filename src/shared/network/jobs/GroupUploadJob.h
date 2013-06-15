/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef GROUP_UPDATE_JOB_H
#define GROUP_UPDATE_JOB_H


#include "HashSet.h"
#include "HashMap.h"
#include "InformingJob.h"
#include "HashString.h"


class ServerObject;
class ServerObjectManager;
class StatusOutput;


class GroupUploadJob : public InformingJob, public InformingJobCallBack {
 public:
								GroupUploadJob(ServerObjectManager* manager,
									const BString& nextChangeSetID,
									BHandler* handler, BMessage* message);
	virtual						~GroupUploadJob();

	// Job interface
 	virtual	status_t			Execute(JobConnection* connection);

	// InformingJobCallBack interface
	virtual status_t			HandleResult(InformingJob* job,
									const BMessage* result);

	// GroupUploadJob
			status_t			InitCheck() const;
			void				WaitUntilDone();

			status_t			AddObject(const BString& id);
			void				Cleanup();

 private:
			void				_RemoveObjectFromManagerAndDisk(
									ServerObject* object) const;

			ServerObjectManager* fObjectManager;
			BString				fNextChangeSetID;

			HashSet<HashString>	fObjects;
			HashMap<HashString, int32> fUploadedObjects;

			ServerObject*		fCurrentObject;

			sem_id				fDoneSem;

			StatusOutput*		fStatusOutput;
};

#endif // GROUP_UPDATE_JOB_H

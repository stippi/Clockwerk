/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "GroupUploadJob.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <DataIO.h>
#include <OS.h>

#include "CommonPropertyIDs.h"
#include "FinishTransactionJob.h"
#include "JobConnection.h"
#include "RemoveJob.h"
#include "RequestMessageCodes.h"
#include "RequestXMLConverter.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "StartTransactionJob.h"
#include "StatusOutput.h"
#include "UploadJob.h"
#include "XMLSupport.h"

using std::nothrow;


// constructor
GroupUploadJob::GroupUploadJob(ServerObjectManager* manager,
		const BString& nextChangeSetID, BHandler* handler, BMessage* message)
	: InformingJob(handler, message)
	, fObjectManager(manager)
	, fNextChangeSetID(nextChangeSetID)

	, fObjects()
	, fUploadedObjects()

	, fCurrentObject(NULL)

	, fDoneSem(create_sem(0, "job finished sem"))

	, fStatusOutput(NULL)
{
}

// destructor
GroupUploadJob::~GroupUploadJob()
{
	delete_sem(fDoneSem);
}

// Execute
status_t
GroupUploadJob::Execute(JobConnection* connection)
{
	fStatusOutput = connection->GetStatusOutput();

	// clean out any previous stuff
	fUploadedObjects.Clear();

	// start a transaction
	StartTransactionJob startTransactionJob(fNextChangeSetID.String(),
		connection->ClientID(), "updater", NULL, NULL);
	startTransactionJob.SetCallBack(this);
	fError = startTransactionJob.Execute(connection);
	if (fError < B_OK) {
		fStatusOutput->PrintErrorMessage("GroupUploadJob: failed to start "
			"transaction\n", strerror(fError));
		return fError;
	}

	// upload all objects that have a path
	HashSet<HashString>::Iterator iterator = fObjects.GetIterator();
	while (iterator.HasNext()) {
		BString id = iterator.Next().GetString();

		fCurrentObject = fObjectManager->FindObject(id);
		if (!fCurrentObject) {
			fStatusOutput->PrintErrorMessage("GroupUploadJob: object with id "
				"'%s' is not in library\n", id.String());
			return fError = B_ERROR;
		}
			
		entry_ref ref;
		fError = fObjectManager->GetRef(id, ref);
		if (fError < B_OK) {
			fStatusOutput->PrintErrorMessage("GroupUploadJob: failed to get "
				"ref for object '%s' (id %s, version %ld): %s\n",
				fCurrentObject->Name().String(), id.String(),
				fCurrentObject->Version(), strerror(fError));
			return fError;
		}

		BMessage metaData;
		fError = fCurrentObject->Archive(&metaData);
		if (fError < B_OK) {
			fStatusOutput->PrintErrorMessage("GroupUploadJob: failed to "
				"archive object '%s' (id %s, version %ld): %s\n",
				fCurrentObject->Name().String(), id.String(),
				fCurrentObject->Version(), strerror(fError));
			return fError;
		}

		int32 maxAttempts = 5;
		int32 attempt = 1;
		while (attempt <= maxAttempts) {
			if (fCurrentObject->Status() == SYNC_STATUS_LOCAL_REMOVED) {
				RemoveJob removeJob(&metaData, NULL, NULL);
		 		removeJob.SetCallBack(this);
		 		fError = removeJob.Execute(connection);
		 		if (fError < B_OK) {
					fStatusOutput->PrintErrorMessage("GroupUploadJob: failed to"
						" remove object '%s' (id %s, version %ld): %s "
						"(attempt %ld of %ld)\n",
						fCurrentObject->Name().String(), id.String(),
						fCurrentObject->Version(), strerror(fError),
						attempt, maxAttempts);
		 		} else
		 			break;
			} else if (fCurrentObject->Status() == SYNC_STATUS_SERVER_REMOVED) {
				// ignore (should not be possible anyways, but don't upload
				// in any case)
			} else {
				UploadJob uploadJob(ref, &metaData, NULL, NULL);
		 		uploadJob.SetCallBack(this);
		 		fError = uploadJob.Execute(connection);
		 		if (fError < B_OK) {
					fStatusOutput->PrintErrorMessage("GroupUploadJob: failed to"
						" upload object '%s' (id %s, version %ld): %s "
						"(attempt %ld of %ld)\n",
						fCurrentObject->Name().String(), id.String(),
						fCurrentObject->Version(), strerror(fError),
						attempt, maxAttempts);
		 		} else
		 			break;
			}
			attempt++;
		}
		if (fError < B_OK) {
			fStatusOutput->PrintErrorMessage("GroupUploadJob: giving up\n");
			return fError;
		}
	}

	fCurrentObject = NULL;

	// finally finish the transaction
	FinishTransactionJob finishTransactionJob(NULL, NULL);
	fError = finishTransactionJob.Execute(connection);
	if (fError < B_OK) {
		fStatusOutput->PrintErrorMessage("GroupUploadJob: failed to finish "
			"transaction: %s\n", strerror(fError));
		return fError;
	}

	// everything went ok, now the objects can be updated with the
	// correct version that we got from the server
	AutoWriteLocker locker(fObjectManager->Locker());
	if (!locker.IsLocked())
		return fError = B_ERROR;

	iterator = fObjects.GetIterator();
	while (iterator.HasNext()) {
		BString id = iterator.Next().GetString();

		fCurrentObject = fObjectManager->FindObject(id);
		if (!fCurrentObject) {
			fStatusOutput->PrintErrorMessage("GroupUploadJob: object with id "
				"'%s' is not in library\n", id.String());
			return fError = B_ERROR;
		}

		if (fCurrentObject->Status() == SYNC_STATUS_LOCAL_REMOVED) {
			_RemoveObjectFromManagerAndDisk(fCurrentObject);
		} else {
			fObjectManager->UpdateVersion(id,
				fUploadedObjects.Get(id.String()));
		}
	}

	// inform the application how the upload worked
	BMessage message(0UL);
	if (fMessage)
		message = *fMessage;

	InformHandler(&message);

	return fError = B_OK;
}

// HandleResult
status_t
GroupUploadJob::HandleResult(InformingJob* job, const BMessage* result)
{
	status_t error;
	if (result->FindInt32("error", &error) >= B_OK) {
		fStatusOutput->PrintErrorMessage("failed to execute a job: %s\n",
			strerror(error));
		return error;
	}

	if (dynamic_cast<UploadJob*>(job)) {
		int32 version;
		if (result->FindInt32("version", &version) < B_OK) {
			fStatusOutput->PrintErrorMessage("didn't obtain version "
				"from server\n");
			return B_ERROR;
		}
		return fUploadedObjects.Put(fCurrentObject->ID().String(), version);
	}
	// NOTE: no need to handle successful RemoveJobs, the objects
	// don't need to be in the fUploadedObjects map

	return B_OK;
}

// InitCheck
status_t
GroupUploadJob::InitCheck() const
{
	if (fDoneSem < 0)
		return fDoneSem;
	return B_OK;
}

// WaitUntilDone
void
GroupUploadJob::WaitUntilDone()
{
	while (acquire_sem(fDoneSem) == B_INTERRUPTED) {}
}

// AddObject
status_t
GroupUploadJob::AddObject(const BString& id)
{
	return fObjects.Add(id.String());
}

// Cleanup
void
GroupUploadJob::Cleanup()
{
	fObjects.Clear();
}

// #pragma mark -

// _RemoveObjectFromManagerAndDisk
void
GroupUploadJob::_RemoveObjectFromManagerAndDisk(ServerObject* object) const
{
	BString id = object->ID();
	if (!fObjectManager->RemoveObject(object))
		return;
	object->Release();

	entry_ref ref;
	if (fObjectManager->GetRef(id, ref) < B_OK)
		return;

	BEntry entry(&ref);
	if (entry.InitCheck() < B_OK    )
		return;

	entry.Remove();
}



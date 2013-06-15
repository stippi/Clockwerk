/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Uploader.h"

#include <new>
#include <string.h>

#include <Alert.h>
#include <Looper.h>

#include "common.h"

#include "AutoDeleter.h"
#include "CommandJob.h"
#include "CommonPropertyIDs.h"
#include "Debug.h"
#include "DownloadJob.h"
#include "FinishTransactionJob.h"
#include "GroupUploadJob.h"
#include "JobConnection.h"
#include "OptionProperty.h"
#include "PropertyObjectFactory.h"
#include "ReferencedObjectFinder.h"
#include "RequestMessageCodes.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "ServerObjectFactory.h"
#include "StartTransactionJob.h"

using std::nothrow;

enum {
	MSG_GO								= 'gogo',
	MSG_GROUP_UPLOAD_RESULT				= 'obgu',
	MSG_CANCEL							= 'cncl',
};

static const int32 kObjectsPerTransaction = 20;
static const int32 kMaxRetries = 3;


// TODO: refactor
class Uploader::MyGroupUploadJob : public GroupUploadJob {
public:
	MyGroupUploadJob(Uploader* uploader, ServerObjectManager* manager,
			const BString& nextChangeSetID)
		: GroupUploadJob(manager, nextChangeSetID,
				uploader, new BMessage(MSG_GROUP_UPLOAD_RESULT)),
		  fUploader(uploader)
	{
	}

	~MyGroupUploadJob()
	{
		if (fUploader->LockLooper()) {
			fUploader->_JobDeleted();
			fUploader->UnlockLooper();
		}
	}

private:
	Uploader*	fUploader;
};

// CancelJob
class Uploader::CancelJob : public Job {
public:
	CancelJob(Uploader* uploader)
		: Job(),
		  fUploader(uploader)
	{
	}

	virtual	status_t Execute(JobConnection* connection)
	{
		StatusOutput* statusOutput = connection->GetStatusOutput();
		statusOutput->PrintWarningMessage("Operation canceled by request.\n");
		return B_OK;
	}

	~CancelJob()
	{
		if (fUploader->LockLooper()) {
			fUploader->_JobDeleted();
			fUploader->UnlockLooper();
		}
	}

private:
	Uploader*	fUploader;
};


// #pragma mark - UploadVisitor

// constructor + destructor
Uploader::UploadVisitor::UploadVisitor() {}
Uploader::UploadVisitor::~UploadVisitor() {}

// Visit
void
Uploader::UploadVisitor::Visit(ServerObject* object, int32 level)
{
}

// #pragma mark - Uploader

// constructor
Uploader::Uploader(JobConnection* connection, ServerObjectManager* manager,
		StatusOutput* statusOutput)
	: BHandler("synchronizer")
	, fConnection(connection)
	, fManager(manager)

	, fNextChangeSetID(1)

	, fOpenJobs(0)

	, fIdNodeMap()
	, fIndegreeZeroNodes(32)
	, fCurrentIndex(-1)
	, fCurrentRetries(0)

	, fInProgress(false)
	, fCanceled(false)

	, fStatusOutput(statusOutput)
{
	if (!fStatusOutput)
		fStatusOutput = &fConsoleOutput;
}

// destructor
Uploader::~Uploader()
{
	// NOTE: 
	// the Uploader needs to be removed from its BLooper
	// then the JobConnection would need to be deleted
	// after that it is ok to delete Uploader
	CleanUp();
}

// MessageReceived
void
Uploader::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case MSG_GO:
			// client initiated asynchronous process
			if (fOpenJobs > 0)
				_ReportFailure(_StartNextGroupUpload());
			else
				_Finished(B_OK);
			break;

		case MSG_GROUP_UPLOAD_RESULT:
			// GroupUploadJob finished
			_ReportFailure(_HandleUploaded(message));
				// will automatically start next upload
				// on success
			break;

		case MSG_CANCEL:
			_HandleCancel(message);
			break;

		default:
			BHandler::MessageReceived(message);
			break;
	}
}

// Init
status_t
Uploader::Init()
{
	return B_OK;
}

// Collect
status_t
Uploader::Collect()
{
	return _CollectObjectsForUpload();
}

// Upload
status_t
Uploader::Upload()
{
	return BMessenger(this).SendMessage(MSG_GO);
}

// Cancel
status_t
Uploader::Cancel()
{
	if (!fInProgress)
		return B_OK;

	return BMessenger(this).SendMessage(MSG_CANCEL);
}

// CleanUp
void
Uploader::CleanUp()
{
	// delete the DepGraphNodes
	IdNodeMap::Iterator iterator = fIdNodeMap.GetIterator();
	while (iterator.HasNext())
		delete iterator.Next().value;

	fIndegreeZeroNodes.MakeEmpty();
	fCurrentIndex = -1;

	fIdNodeMap.Clear();
	fInProgress = false;
	fCanceled = false;
	fOpenJobs = 0;
}

// VisitUploads
status_t
Uploader::VisitUploads(Uploader::UploadVisitor* visitor) const
{
	if (!visitor)
		return B_BAD_VALUE;

	IdNodeMap::Iterator iterator = fIdNodeMap.GetIterator();
	while (iterator.HasNext())
		visitor->Visit(iterator.Next().value->object, 0);

	return B_OK;
}

// #pragma mark -

// _CollectObjectsForUpload
status_t
Uploader::_CollectObjectsForUpload()
{
	if (!fManager->ReadLock())
		RETURN_ERROR(B_ERROR);

	// build node graph
	int32 count = fManager->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fManager->ObjectAtFast(i);
		BString serverID = object->ID();
		int32 syncStatus = object->Status();
		if (syncStatus == SYNC_STATUS_PUBLISHED
			|| syncStatus == SYNC_STATUS_SERVER_REMOVED) {
//printf("object at %s is up to date\n", serverID.String());
			continue;
		}
{
const char* status;
switch (syncStatus) {
	case SYNC_STATUS_LOCAL: status = "Local"; break;
	case SYNC_STATUS_MODIFIED: default: status = "Modified"; break;
	case SYNC_STATUS_LOCAL_REMOVED: status = "Removed"; break;
}
BString name = object->Name();
printf("object at %s (%s) needs to be uploaded (has status: %s)\n",
	serverID.String(), name.String(), status);
}
		// this object is local/modified
		DepGraphNode* node = new (nothrow) DepGraphNode(object);
		if (!node)
			RETURN_ERROR(B_NO_MEMORY);

		fIdNodeMap.Put(serverID.String(), node);
	}

	fManager->ReadUnlock();

	// build the dependencies and increase indegree values
	IdNodeMap::Iterator iterator = fIdNodeMap.GetIterator();
	while (iterator.HasNext())
		_FindReferencedObjects(iterator.Next().value);

	// build the list of objects sorted by indegree
	// as the first set, add all the objects that already have
	// an indegree of zero
	iterator = fIdNodeMap.GetIterator();
	while (iterator.HasNext()) {
		DepGraphNode* node = iterator.Next().value;
		if (node->indegree == 0) {
			status_t ret = _AddNodeToSortedList(node);
			if (ret < B_OK)
				RETURN_ERROR(ret);
		}
	}

	// init job progress
	fCurrentIndex = fIndegreeZeroNodes.CountItems();
	fOpenJobs = (fCurrentIndex + kObjectsPerTransaction - 1)
		/ kObjectsPerTransaction;
	fCanceled = false;

//	count = fIndegreeZeroNodes.CountItems();
//	for (int32 i = 0; i < count; i++) {
//		DepGraphNode* node = (DepGraphNode*)fIndegreeZeroNodes.ItemAtFast(i);
//		printf("%ld -> %s\n", i, node->object->Name().String());
//	}

	if (fOpenJobs == 0)
		fStatusOutput->PrintInfoMessage("Nothing to do.\n");

	return B_OK;
}

// _StartNextGroupUpload
status_t
Uploader::_StartNextGroupUpload()
{
	// build the next changeset id
	// TODO: maybe something better here
	BString nextChangeSetID(fConnection->ClientID());
	nextChangeSetID << system_time() << ':' << fNextChangeSetID++;

	// construct job
	MyGroupUploadJob* uploadJob = new (nothrow) MyGroupUploadJob(this, 
		fManager, nextChangeSetID);
	if (!uploadJob)
		RETURN_ERROR(B_NO_MEMORY);

	// collect the next group of objects from the end of the
	// sorted dependency list
	int32 index = max_c(0, fCurrentIndex - kObjectsPerTransaction);

	for (int32 i = index; i < fCurrentIndex; i++) {
		DepGraphNode* node = (DepGraphNode*)fIndegreeZeroNodes.ItemAt(i);
		status_t ret = uploadJob->AddObject(node->object->ID());
		if (ret < B_OK)
			RETURN_ERROR(ret);
	}

	return fConnection->ScheduleJob(uploadJob, true);
}

// _HandleUploaded
status_t
Uploader::_HandleUploaded(BMessage* message)
{
	status_t error;
	if (message->FindInt32("error", &error) >= B_OK) {
		fStatusOutput->PrintErrorMessage("failed to upload a "
			"transaction: %s\n", strerror(error));
		fCurrentRetries++;
		if (fCurrentRetries < kMaxRetries) {
			fStatusOutput->PrintWarningMessage("retrying\n");
			fOpenJobs++;
			return _StartNextGroupUpload();
		}
		fStatusOutput->PrintErrorMessage("giving up\n");
		return error;
	}

	fCurrentRetries = 0;

	fCurrentIndex -= kObjectsPerTransaction;
	if (fCurrentIndex <= 0) {
		// done
		return B_OK;
	}

	if (!fCanceled)
		return _StartNextGroupUpload();
	return B_OK;
}

// _HandleCancel
status_t
Uploader::_HandleCancel(BMessage* message)
{
	if (fCanceled) {
		fConnection->GetStatusOutput()->PrintWarningMessage(
			"[Patience please]");
		return B_OK;
	}

	// create "cancel" job
	CancelJob* job = new(nothrow) CancelJob(this);

	// schedule it
	status_t error = fConnection->ScheduleJob(job, true);
	if (error != B_OK) {
		fStatusOutput->PrintErrorMessage("Failed to schedule cancel job\n");
		fInProgress = false;
		RETURN_ERROR(error);
	}

	fConnection->GetStatusOutput()->PrintWarningMessage("[Stopping after "
		"current transaction]");
	fCanceled = true;

	return B_OK;
}

// _JobDeleted
void
Uploader::_JobDeleted()
{
	fOpenJobs--;
	if (fOpenJobs == 0)
		_Finished(B_OK);
}

// _ReportFailure
void
Uploader::_ReportFailure(status_t error)
{
	if (error == B_OK)
		return;

	_Finished(error);
}

// _Finished
void
Uploader::_Finished(status_t error)
{
	fOpenJobs = 0;
	fInProgress = false;

	CleanUp();

	// nothing left to do, inform controller app
	BMessage message(MSG_ALL_OBJECTS_UPLOADED);
	if (error != B_OK)
		message.AddInt32("error", error);

	if (fCanceled) {
		message.AddBool("canceled", true);
		fCanceled = false;
	}

	Looper()->PostMessage(&message);
}

// _FindReferencedObjects
status_t
Uploader::_FindReferencedObjects(DepGraphNode* node)
{
	// find all the direct dependency objects for this node
	IdSet ids;
	status_t ret = ReferencedObjectFinder::FindReferencedObjects(fManager,
		node->object, ids);
	if (ret < B_OK)
		return ret;

	// iterate over the dependencies, but only those are interesting
	// that need to be uploaded (are contained in the node map)
	IdSet::Iterator iterator = ids.GetIterator();
//if (!iterator.HasNext())
//printf("object %s has no dependencies\n", node->object->Name().String());

	while (iterator.HasNext()) {
		HashString id = iterator.Next();
		if (fIdNodeMap.ContainsKey(id)) {
			// object is conained in node map (needs to be uploaded)
			DepGraphNode* successor = fIdNodeMap.Get(id);
			if (successor == node) {
//printf("cyclic dependency\n");
				// cyclic dependency, ignore
				continue;
			}
			// add it to successors of initial node and increment
			// the indegree count
			if (!node->successors.HasItem(successor)) {
//printf("adding %s (indegree: %ld) as successor to %s\n",
//successor->object->Name().String(), successor->indegree,
//node->object->Name().String());
				if (node->successors.AddItem(successor))
					successor->indegree++;
				else
					return B_NO_MEMORY;
				// recurse for the successor
				_FindReferencedObjects(successor);
			}
//else
//printf("successor already added!\n");
		}
//else
//printf("object %s is already uploaded\n", id.GetString());
	}

	return B_OK;
}

// _AddNodeToSortedList
status_t
Uploader::_AddNodeToSortedList(DepGraphNode* node)
{
	if (fIndegreeZeroNodes.HasItem(node))
		return B_OK;

	if (!fIndegreeZeroNodes.AddItem(node))
		RETURN_ERROR(B_NO_MEMORY);

	int32 count = node->successors.CountItems();
	for (int32 i = 0; i < count; i++) {
		DepGraphNode* successor
			= (DepGraphNode*)node->successors.ItemAtFast(i);
		successor->indegree--;
		if (successor->indegree == 0) {
			status_t ret = _AddNodeToSortedList(successor);
			if (ret < B_OK)
				RETURN_ERROR(ret);
		}
	}

	return B_OK;
}

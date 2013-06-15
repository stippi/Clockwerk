/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Synchronizer.h"

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
#include "GetRevisionJob.h"
#include "JobConnection.h"
#include "ListFilesJob.h"
#include "Logger.h"
#include "OptionProperty.h"
#include "PropertyObjectFactory.h"
#include "ReferencedObjectFinder.h"
#include "RequestMessageCodes.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "ServerObjectFactory.h"
#include "StartTransactionJob.h"
#include "UploadJob.h"

using std::nothrow;


static Logger sLog("Synchronizer");


enum {
	MSG_CANCEL_OPERATION	= 'cncl'
};


class Synchronizer::MyDownloadJob : public DownloadJob {
public:
	MyDownloadJob(Synchronizer* synchronizer, const BString& serverID,
			int32 version)
		: DownloadJob(synchronizer->fManager->Directory(), serverID,
				version, synchronizer, new BMessage(MSG_OBJECT_DOWNLOADED)),
		  fSynchronizer(synchronizer)
	{
	}

	~MyDownloadJob()
	{
		if (fSynchronizer->LockLooper()) {
			fSynchronizer->_JobDeleted(ServerID());
			fSynchronizer->UnlockLooper();
		}
	}

private:
	Synchronizer*	fSynchronizer;
};


class Synchronizer::CancelJob : public Job {
public:
	CancelJob(Synchronizer* synchronizer)
		: Job(),
		  fSynchronizer(synchronizer)
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
		if (fSynchronizer->LockLooper()) {
			fSynchronizer->_JobDeleted("");
			fSynchronizer->UnlockLooper();
		}
	}

private:
	Synchronizer*	fSynchronizer;
};


class ErrorsOccuredSetter {
public:
	ErrorsOccuredSetter(bool& errorFlag)
		: fErrorFlag(errorFlag)
		, fDetached(false)
	{
	}
	~ErrorsOccuredSetter()
	{
		if (!fDetached)
			fErrorFlag = true;
	}
	void Detach()
	{
		fDetached = true;
	}
private:
	bool&	fErrorFlag;
	bool	fDetached;
};


// #pragma mark -

Synchronizer::ObjectFilter::ObjectFilter() {}
Synchronizer::ObjectFilter::~ObjectFilter() {}

bool
Synchronizer::ObjectFilter::WantsObject(const BString& type, const BString& name,
	int32 version)
{
	return true;
}

void
Synchronizer::ObjectFilter::PrepareObject(ServerObject* object)
{
}

bool
Synchronizer::ObjectFilter::WantsDependencies(const ServerObject* object)
{
	return true;
}


// #pragma mark -

// constructor
Synchronizer::Synchronizer(JobConnection* connection,
		ServerObjectManager* manager, ServerObjectFactory* factory,
		const BString& downloadScope, bool downloadAllObjects, bool testMode,
		StatusOutput* statusOutput)
	: BHandler("synchronizer")
	, fConnection(connection)
	, fManager(manager)
	, fFactory(factory)
	, fObjectFilter(NULL)
	, fDownloadScope(downloadScope)
	, fNextChangeSetID(1)

	, fClientLatestRevision(0)
	, fClientLocalRevision(0)

	, fServerLatestRevision(-1)
	, fServerLocalRevision(-1)

	, fMissingIDs()
	, fOldIDs()
	, fFinalMissingIDs()
	, fLocallyExistingMissingOnServerIDs()
	, fOpenJobs(0)

	, fInProgress(false)
	, fCanceled(false)
	, fDownloadAllObjects(downloadAllObjects)
	, fTestMode(testMode)
	, fConsumeLessCPU(false)

	, fObjectsChanged(false)
	, fErrorsOccured(false)
	, fFirstDownload(true)

	, fStatusOutput(statusOutput)
{
	if (!fStatusOutput)
		fStatusOutput = &fConsoleOutput;
}

// destructor
Synchronizer::~Synchronizer()
{
	// NOTE: 
	// the Synchronizer needs to be removed from its BLooper
	// then the JobConnection would need to be deleted
	// after that it is ok to delete Synchronizer
}

// MessageReceived
void
Synchronizer::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case MSG_SYNCHRONIZE:
			 _QueryRevision(fClientLatestRevision);
			break;

		case MSG_REVISION_RESULT:
			// list of server change sets and latest revision
			_HandleRevisionResult(message);
			break;

		case MSG_QUERY_RESULT:
			// list of server object ids
			_HandleQueryResult(message);
			break;

		case MSG_OBJECT_DOWNLOADED:
			// DownloadJob finished
			_HandleDownloaded(message);
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
Synchronizer::Init()
{
	status_t ret = fMissingIDs.InitCheck();
	if (ret < B_OK)
		RETURN_ERROR(ret);

	return B_OK;
}

// SetObjectFilter
void
Synchronizer::SetObjectFilter(ObjectFilter* filter)
{
	fObjectFilter = filter;
}

// SetConsumeLessCPU
void
Synchronizer::SetConsumeLessCPU(bool consumeLess)
{
	fConsumeLessCPU = consumeLess;
}

// Update
status_t
Synchronizer::Update(int64 clientLatestRevision, int64 clientLocalRevision)
{
	fClientLatestRevision = clientLatestRevision;
	fClientLocalRevision = clientLocalRevision;
	return BMessenger(this).SendMessage(MSG_SYNCHRONIZE);
}

// Cancel
status_t
Synchronizer::Cancel()
{
	if (!fInProgress)
		return B_OK;

	return BMessenger(this).SendMessage(MSG_CANCEL);
}

// CleanUp
void
Synchronizer::CleanUp()
{
	fInProgress = false;
	fCanceled = false;
	fErrorsOccured = false;
	fFirstDownload = true;
	fOpenJobs = 0;

	fIDVersionMap.Clear();
	fMissingIDs.Clear();
	fOldIDs.Clear();
	fFinalMissingIDs.Clear();
	fLocallyExistingMissingOnServerIDs.Clear();
}

// #pragma mark -

// _QueryRevision
status_t
Synchronizer::_QueryRevision(int64 clientRevision)
{
	if (fInProgress)
		return B_BUSY;

	LOG_DEBUG("_QueryRevision() ...\n");

	// create "get revision" job
	GetRevisionJob* job = new(nothrow) GetRevisionJob(clientRevision, this,
		new BMessage(MSG_REVISION_RESULT));

	// schedule it
	status_t error = fConnection->ScheduleJob(job, true);
	if (error != B_OK) {
		fStatusOutput->PrintErrorMessage("Failed to schedule revision job\n");
		LOG_ERROR("_QueryRevision(): Failed to schedule job: %s\n",
			strerror(error));
		RETURN_ERROR(error);
	}

	fInProgress = true;

	return B_OK;
}

// _HandleRevisionResult
status_t
Synchronizer::_HandleRevisionResult(BMessage* message)
{
	if (!fInProgress)
		return B_OK;

	LOG_DEBUG("_HandleRevisionResult() ...\n");

	status_t error;
	if (message->FindInt32("error", &error) >= B_OK) {
		fStatusOutput->PrintErrorMessage("failed to query server "
			"revision: %s\n", strerror(error));
		LOG_ERROR("_HandleRevisionResult(): failed to query server "
			"revision: %s\n", strerror(error));
		fInProgress = false;
		RETURN_ERROR(error)
	}

	// get the message with the server changesets
	// (sent by the GetRevisionJob)
	BMessage revision;
	status_t status = message->FindMessage("revision", &revision);
	if (status < B_OK) {
		LOG_ERROR("_HandleRevisionResult(): failed to get revision\n");
		fInProgress = false;
		RETURN_ERROR(status)
	}

	// find out latest server revision
	// there is nothing to do for us if the server revision is
	// the same as the latest client revision
	int64 serverLatestRevision;
	int64 serverLocalRevision;
	if (revision.FindInt64("latest revision number",
			&serverLatestRevision) == B_OK
		&& revision.FindInt64("local revision number",
			&serverLocalRevision) == B_OK) {
		fServerLatestRevision = serverLatestRevision;
		fServerLocalRevision = serverLocalRevision;
		if (fServerLatestRevision == fClientLatestRevision
			&& fServerLocalRevision == fClientLocalRevision) {
			fOpenJobs = 1;
			_AllDownloadsDone();
			return B_OK;
		}
		fStatusOutput->PrintInfoMessage("synchronization needed: server "
			"revision: %lld/%lld, local: %lld/%lld\n", fServerLatestRevision,
			fServerLocalRevision, fClientLatestRevision, fClientLocalRevision);
		LOG_INFO("_HandleRevisionResult(): synchronization needed: server "
			"revision: %lld/%lld, local: %lld/%lld\n", fServerLatestRevision,
			fServerLocalRevision, fClientLatestRevision, fClientLocalRevision);
	} else {
		LOG_ERROR("_HandleRevisionResult(): failed to get latest/local revision"
			" number\n");
	}

	return _QueryObjects();
}

// _QueryObjects
status_t
Synchronizer::_QueryObjects()
{
	LOG_DEBUG("_QueryObjects() ...\n");

	// create "list files" job
	ListFilesJob* job = new(nothrow) ListFilesJob(fDownloadScope, this,
		new BMessage(MSG_QUERY_RESULT));

	// schedule it
	status_t error = fConnection->ScheduleJob(job, true);
	if (error != B_OK) {
		fStatusOutput->PrintErrorMessage("Failed to schedule list job\n");
		LOG_ERROR("_QueryObjects(): failed to schedule job: %s\n",
			strerror(error));
		fInProgress = false;
		RETURN_ERROR(error);
	}

	return B_OK;
}

// _HandleQueryResult
status_t
Synchronizer::_HandleQueryResult(BMessage* message)
{
	if (!fInProgress)
		return B_OK;

	LOG_DEBUG("_HandleQueryResult() ...\n");

	status_t error;
	if (message->FindInt32("error", &error) >= B_OK) {
		fStatusOutput->PrintErrorMessage("failed to query server "
			"objects: %s\n", strerror(error));
		LOG_ERROR("_HandleQueryResult(): failed to query server "
			"objects: %s\n", strerror(error));
		fInProgress = false;
		RETURN_ERROR(error)
	}

	// get the message with the server ids
	// (sent by the ListFilesJob)
#ifndef LIST_FILES_JOB_REPLY_IS_LISTING
	BMessage listing;
	status_t status = message->FindMessage("listing", &listing);
	if (status < B_OK) {
		LOG_ERROR("_HandleQueryResult(): failed to get listing\n");
		fInProgress = false;
		RETURN_ERROR(status)
	}
#else
	BMessage& listing = *message;
	status_t status = B_OK;
#endif

	if (!fManager->ReadLock()) {
		LOG_ERROR("_HandleQueryResult(): failed to lock manager\n");
		fInProgress = false;
		RETURN_ERROR(B_ERROR);
	}

	fIDVersionMap.Clear();
	fMissingIDs.Clear();
	fFinalMissingIDs.Clear();
	fLocallyExistingMissingOnServerIDs.Clear();
	fOldIDs.Clear();
	fErrorsOccured = false;
	fFirstDownload = true;
	fCanceled = false;

	// put all current objects into the "old" set
	// later, "downloaded" objects are removed from
	// this set, everything still in it is then
	// considered "obsolete"
	int32 count = fManager->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fManager->ObjectAtFast(i);
		BString id = object->ID();
		status = fOldIDs.Add(id.String());
		if (status < B_OK) {
			fStatusOutput->PrintErrorMessage("Out of memory for "
				"adding id to \"old\" hash set!\n");
			LOG_ERROR("_HandleQueryResult(): Out of memory for "
				"adding id to \"old\" hash set!\n");
			break;
		}
	}

	BString serverID;
	int32 version;
	BString type;

	// check message for conversion errors
	type_code typeFound;
	int32 soidCountFount;
	listing.GetInfo("soid", &typeFound, &soidCountFount);
	int32 vrsnCountFount;
	listing.GetInfo("vrsn", &typeFound, &vrsnCountFount);
	int32 typeCountFount;
	listing.GetInfo("type", &typeFound, &typeCountFount);
	if (soidCountFount != vrsnCountFount || soidCountFount != typeCountFount) {
		print_error("listing BMessage entry counts do not match: "
			"%ld soid, %ld vrsn, %ld type\n",
			soidCountFount, vrsnCountFount, typeCountFount);
		LOG_ERROR("_HandleQueryResult(): listing BMessage entry counts do not "
			"match: %ld soid, %ld vrsn, %ld type\n",
			soidCountFount, vrsnCountFount, typeCountFount);
		// TODO: should we even proceed?
		fErrorsOccured = true;
	}

	for (int32 i = 0; listing.FindString("soid", i, &serverID) == B_OK
			&& listing.FindInt32("vrsn", i, &version) == B_OK
			&& listing.FindString("type", i, &type) == B_OK; i++) {
		if (serverID.Length() < 5 || version > 1000 || version < 0
			|| type.Length() < 4) {
			print_warning("implausible data in listing at index %ld: "
				"%s, version: %ld, type: %s\n",
				i, serverID.String(), version, type.String());
			LOG_WARN("_HandleQueryResult(): implausible data in listing at "
				"index %ld: %s, version: %ld, type: %s\n",
				i, serverID.String(), version, type.String());
			fErrorsOccured = true;
		}

		BString name;
		if (listing.FindString("name", i, &name) < B_OK)
			name = "<unkown>";

		bool addToMissing = true;
		if (fObjectFilter)
			addToMissing = fObjectFilter->WantsObject(type, name, version);

		LOG_TRACE("_HandleQueryResult(): listing entry %ld: %s, version: %ld "
			"(missing: %d)\n", i, serverID.String(), version, addToMissing);

		if (addToMissing) {
//printf("add to missing: %s\n", serverID.String());
			status = fMissingIDs.Add(serverID.String());
			if (status < B_OK) {
				fStatusOutput->PrintErrorMessage("Out of memory for "
					"adding id to hash set!\n");
				LOG_ERROR("_HandleQueryResult(): Out of memory for "
					"adding id to hash set!\n");
				break;
			}
		}

//printf("%s -> version: %ld\n", serverID.String(), version);
		status = fIDVersionMap.Put(serverID.String(), version);
		if (status < B_OK) {
			fStatusOutput->PrintErrorMessage("Out of memory for "
				"adding id/version to hash map!\n");
			LOG_ERROR("_HandleQueryResult(): Out of memory for "
				"adding id/version to hash map!\n");
			break;
		}
	}

	fOpenJobs++;

	fManager->ReadUnlock();

	_FetchNextMissingObject();

	RETURN_ERROR(status);
}

// _FetchNextMissingObject
status_t
Synchronizer::_FetchNextMissingObject()
{
	while (true) {
		if (fMissingIDs.IsEmpty()) {
			_AllDownloadsDone();
			return B_OK;
		}
		if (fConsumeLessCPU)
			snooze(1000);

		BString serverID = fMissingIDs.GetIterator().Next().GetString();

		LOG_DEBUG("_FetchNextMissingObject(): %s\n", serverID.String());

		// this object is not obsolete
		fOldIDs.Remove(serverID.String());

		ServerObject* object = fManager->FindObject(serverID);
		if (object && object->Status() == SYNC_STATUS_SERVER_REMOVED) {
			LOG_INFO("_FetchNextMissingObject(): object %s was marked "
				"removed, but is back again\n", serverID.String());

			// this object was already downloaded, but marked
			// removed. In the new listing, it re-appeared,
			// so we unmark it...
			object->SetStatus(SYNC_STATUS_PUBLISHED);
			// ... and make sure that the controller
			// rescans the library for a suitable playlist
			_ObjectsChanged();
		}

		if (!fIDVersionMap.ContainsKey(serverID.String())) {
			if (object) {
				fLocallyExistingMissingOnServerIDs.Add(serverID.String());
				if (fDownloadAllObjects) {
					fStatusOutput->PrintWarningMessage("WARNING: Referenced "
						"object %s (%s/%s) is NOT on the server (but a local "
						"copy exists) - ignoring!\n",
						serverID.String(), object->Type().String(),
						object->Name().String());
					LOG_WARN("_FetchNextMissingObject(): Referenced "
						"object %s (%s/%s) is NOT on the server (but a local "
						"copy exists) - ignoring!\n",
						serverID.String(), object->Type().String(),
						object->Name().String());
				}
			} else {
				fStatusOutput->PrintWarningMessage("WARNING: Referenced object "
					"%s is not in local library and NOT on the server - "
					"ignoring!\n", serverID.String());
				LOG_WARN("_FetchNextMissingObject(): Referenced object "
					"%s is not in local library and NOT on the server - "
					"ignoring!\n", serverID.String());
				fFinalMissingIDs.Add(serverID.String());
			}
			fMissingIDs.Remove(serverID.String());
			continue;
		}

		if (!object
			|| object->Version() != fIDVersionMap.Get(serverID.String())) {
			int32 serverVersion = fIDVersionMap.Get(serverID.String());

			LOG_INFO("_FetchNextMissingObject(): Need to download object %s, "
				"version %ld (local version: %ld)\n", serverID.String(),
				serverVersion, object ? object->Version() : 0);

//{
//if (!object)
//	printf("object %s not in local library\n", serverID.String());
//else {
// 	if (object->Version() != serverVersion)
//		printf("local object %s (%s/%s) is old version %ld, new version: %ld\n",
//				serverID.String(), object->Type().String(),
//				object->Name().String(), object->Version(), serverVersion);
//printf("scheduling download for %s\n", serverID.String());
//}
//}	
			if (fDownloadAllObjects && object
				&& object->Status() != SYNC_STATUS_PUBLISHED) {
				BString message("The object\n\n");
				message << "Name: " << object->Name() << "\n";
				message << "Type: " << object->Type() << "\n";
				message << "ID: " << object->ID() << "\n";
				message << "Version: " << object->Version() << "\n\n";
				message << "has LOCAL MODIFICATIONS, but a newer version (";
				message << serverVersion <<") exists on the server.\n\n";
				message << "What should be done with the local object?";
				BAlert* alert = new BAlert("collision", message.String(),
					"Overwrite", "Keep Local");
				// make sure the alert is showing...
				// TODO: put somewhere in support functions
				uint32 workspaces = alert->Workspaces();
				int32 workspaceIndex = 0;
				for (; workspaceIndex < 32; workspaceIndex++) {
					if (workspaces & (1 << workspaceIndex) != 0) {
						activate_workspace(workspaceIndex);
						break;
					}
				}
				// TODO: option to not ask again!
				int32 choice = alert->Go();
				if (choice == 1) {
					fStatusOutput->PrintWarningMessage("Kept "
						"local modified object '%s' of type %s, version %ld, "
						"ignored newer server version %ld\n",
						object->Name().String(), object->Type().String(),
						object->Version(), serverVersion);

//					object->SetVersion(serverVersion);
					fMissingIDs.Remove(serverID.String());
					// NOTE: this happens only in editor mode, and so
					// we don't need to search for referenced objects
					// of this object for download, since we download
					// all objects anyways
					continue;
				} else {
					fStatusOutput->PrintWarningMessage("Overwriting "
						"local modifications of object '%s' of type %s, "
						"version %ld with newer server version %ld\n",
						object->Name().String(), object->Type().String(),
						object->Version(), serverVersion);
				}
			}

			// tell the user into which folder we are downloading if
			// this is the first download
			if (fFirstDownload) {
				fStatusOutput->PrintInfoMessage("Downloading into folder "
					"'%s'.\n", fManager->Directory());
				fFirstDownload = false;
			}

			// create a download job for this new id (or new version)
			DownloadJob* job = new(nothrow) MyDownloadJob(this, serverID,
				serverVersion);
			status_t status = fConnection->ScheduleJob(job, true);
			if (status != B_OK) {
				fMissingIDs.Remove(serverID.String());
				
				fStatusOutput->PrintErrorMessage("Failed to schedule "
					"download job for file \"%s\"\n", serverID.String());
				LOG_ERROR("_FetchNextMissingObject(): Failed to schedule "
					"download job for object \"%s\"\n", serverID.String());
			} else {
				fOpenJobs++;
					// TODO: this flag doesn't _really_ say
					// if there is something left to do, since
					// that would also be the case if something
					// failed earlier
				// see you in _HandleDownloaded();
				return B_OK;
			}
		} else {
			// we have this object with the correct version locally,
			// but we need to make sure we also have every object that
			// it references
			LOG_DEBUG("_FetchNextMissingObject(): object %s "
				"up to date (version: %ld)\n", serverID.String(),
				object->Version());

			fMissingIDs.Remove(serverID.String());
			status_t status = _FindReferencedObjects(object);
			if (status < B_OK) {
				fErrorsOccured = true;
				fStatusOutput->PrintErrorMessage("Failed to find "
					"referenced objects for id \"%s\"\n", serverID.String());
				LOG_ERROR("_FetchNextMissingObject(): Failed to find "
					"referenced objects for id \"%s\"\n", serverID.String());
			}
		}
	}
}

// _FindReferencedObjects
status_t
Synchronizer::_FindReferencedObjects(const ServerObject* object)
{
	if (fDownloadAllObjects)
		return B_OK;
	// NOTE: this object is expected to be available already

	if (fObjectFilter && !fObjectFilter->WantsDependencies(object)) {
		LOG_DEBUG("_FetchNextMissingObject(): object filter doesn't want "
			"dependencies for object \"%s\"\n", object->ID().String());
		return B_OK;
	}

	// find referenced object
	return ReferencedObjectFinder::FindReferencedObjects(fManager, object,
		fMissingIDs);
}

// _InsertDownloadedObject
status_t
Synchronizer::_InsertDownloadedObject(BMessage* message, ServerObject** _object)
{
	*_object = NULL;

	ErrorsOccuredSetter errorSetter(fErrorsOccured);

	status_t error;
	if (message->FindInt32("error", &error) >= B_OK) {
		fStatusOutput->PrintErrorMessage("Failed to download an "
			"object: %s\n", strerror(error));
		LOG_ERROR("_InsertDownloadedObject(): Failed to download an "
			"object: %s\n", strerror(error));
		// TODO: somehow retry here?
		return error;
	}

	// find meta data message and reconstruct a ServerObject from it
	BMessage metaData;

	status_t status = message->FindMessage("meta data", &metaData);
	if (status < B_OK) {
		LOG_ERROR("_InsertDownloadedObject(): no meta data\n");
		RETURN_ERROR(status);
	}

	BString type;
	status = metaData.FindString("type", &type);
	if (status < B_OK) {
		LOG_ERROR("_InsertDownloadedObject(): no type\n");
		RETURN_ERROR(status);
	}

	BString id;
	status = metaData.FindString("soid", &id);
	if (status < B_OK) {
		LOG_ERROR("_InsertDownloadedObject(): no object ID\n");
		RETURN_ERROR(status);
	}

	// NOTE: after instantiating this object with a pointer to our
	// current object library, it might obviously reference existing
	// objects that might yet be updated (in a later download job).
	// But this is no problem, since we then update the objects in place,
	// so the pointers don't become stale
	ServerObject* object = fFactory->Instantiate(type, id, fManager);
	status = object ? object->InitCheck() : B_NO_MEMORY;
	if (status < B_OK) {
		delete object;
		LOG_ERROR("_InsertDownloadedObject(): Failed to instantiate object "
			"%s\n", id.String());
		RETURN_ERROR(status);
	}

	if (fObjectFilter)
		fObjectFilter->PrepareObject(object);

	ObjectDeleter<ServerObject> deleter(object);

	// the object data is loaded, but the attributes could not
	// be loaded, since the DownloadJob doesn't attach those to the file!
	// luckily, we have the meta data available in the message archive
	status = object->Unarchive(&metaData);
	if (status < B_OK) {
		LOG_ERROR("_InsertDownloadedObject(): Failed to unarchive meta data for "
			"object %s\n", id.String());
		RETURN_ERROR(status);
	}

	if (!fManager->WriteLock()) {
		LOG_ERROR("_InsertDownloadedObject(): Failed to lock manager.");
		RETURN_ERROR(B_ERROR);
	}

	// update server object manager
	// (either by updating an existing object or by adding the object)
	ServerObject* original = fManager->FindObject(object->ID());
	if (original) {
//printf("found original object, updating\n");
//printf("  old version: %ld, new version: %ld\n", original->Version(), object->Version());
		LOG_DEBUG("_InsertDownloadedObject(): Updating object %s, version: "
			"%ld -> %ld\n", id.String(), original->Version(),
			object->Version());
		// update the object instance
		original->SetTo(object);
		original->SetDependenciesResolved(false);

		*_object = original;
		// make sure the object meta data state is saved persistently
		// NOTE: the object data itself was already saved by the DownloadJob
		fManager->StateChanged();
	} else {
//printf("adding new object\n");
		LOG_DEBUG("_InsertDownloadedObject(): Adding new object %s, version: "
			"%ld\n", id.String(), object->Version());
		if (!fManager->AddObject(object)) {
			LOG_ERROR("_InsertDownloadedObject(): Failed to add object object "
				"%s, version: %ld\n", id.String(), object->Version());
			status = B_NO_MEMORY;
		} else {
			*_object = object;
			deleter.Detach();
		}
	}

	fManager->WriteUnlock();

	if (status < B_OK)
		RETURN_ERROR(status);

	errorSetter.Detach();

	return B_OK;
}


// _HandleDownloaded
status_t
Synchronizer::_HandleDownloaded(BMessage* message)
{
	ServerObject* object;
	status_t status = _InsertDownloadedObject(message, &object);

	if (status == B_OK) {
		if (object != NULL) {
			// we actually inserted a new object
			// or updated an existing object
			_ObjectsChanged();
			status = _FindReferencedObjects(object);
			if (status < B_OK) {
				LOG_ERROR("_HandleDownloaded(): Failed to find "
					"referenced objects for id \"%s\"\n",
					object->ID().String());
				fErrorsOccured = true;
				REPORT_ERROR(status);
			}
		} // else
		  // we didn't insert an object, but we should still
		  // continue downloading more objects
	} else
		REPORT_ERROR(status);

	if (!fCanceled)
		return _FetchNextMissingObject();
	return B_OK;
}

// _AllDownloadsDone
void
Synchronizer::_AllDownloadsDone()
{
	if (!fTestMode && !fErrorsOccured)
		_HandleObsoleteObjects();

	// remove fake job count, which triggers the
	// "synchronization complete" notification
	_JobDeleted("");
}

// _HandleCancel
status_t
Synchronizer::_HandleCancel(BMessage* message)
{
	LOG_DEBUG("_HandleCancel() ...\n");

	if (fCanceled) {
		fConnection->GetStatusOutput()->PrintWarningMessage("[Patience please]");
		return B_OK;
	}

	// create "cancel" job
	CancelJob* job = new(nothrow) CancelJob(this);

	// schedule it
	status_t error = fConnection->ScheduleJob(job, true);
	if (error != B_OK) {
		fStatusOutput->PrintErrorMessage("Failed to schedule cancel job\n");
		LOG_ERROR("_HandleCancel(): failed to schedule job: %s\n",
			strerror(error));
		fInProgress = false;
		RETURN_ERROR(error);
	}

	fConnection->GetStatusOutput()->PrintWarningMessage("[Stopping after "
		"download]");
	fCanceled = true;

	return B_OK;
}

// _JobDeleted
void
Synchronizer::_JobDeleted(const BString& serverID)
{
	// remove from download set
	// NOTE: if the id was removed even though
	// the download actually failed, it will
	// be inserted again at the next query (pulse)
	fMissingIDs.Remove(serverID.String());

	fOpenJobs--;
	if (fOpenJobs == 0) {
		LOG_DEBUG("_JobDeleted(): all jobs done, errors occured: %d\n",
			fErrorsOccured);

		fInProgress = false;

		// nothing left to do, inform controller app
		BMessage message(MSG_SYNCHRONIZED);
		if (fObjectsChanged) {
			// to be on the save side,
			// trigger saving the object library to disk
			fManager->StateChanged();
			fObjectsChanged = false;
			// this causes the controller to search for
			// a new suitable playlist
			message.AddBool("check objects", true);
		}
		if (fErrorsOccured) {
			message.AddBool("errors occured", true);
			fErrorsOccured = false;
		}
		if (fCanceled) {
			message.AddBool("canceled", true);
			fCanceled = false;
		}
		Looper()->PostMessage(&message);
	}
}

// _HandleObsoleteObjects
void
Synchronizer::_HandleObsoleteObjects()
{
	// NOTE: manager already read-locked
	int32 count = fManager->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fManager->ObjectAtFast(i);
		BString id = object->ID();
		BString name = object->Name();
		if (fOldIDs.Contains(id.String())
			&& object->Status() == SYNC_STATUS_PUBLISHED) {
			if (fDownloadAllObjects) {
				// this object is not on the server, but marked as published,
				// make sure it is marked local, so that it is uploaded
fStatusOutput->PrintInfoMessage("published object '%s', '%s' type %s, "
	"v %ld is not on the server anymore.\n", id.String(), name.String(),
	object->Type().String(), object->Version());
				LOG_DEBUG("_HandleObsoleteObjects(): published object "
					" '%s', '%s' type '%s', v %ld is not on the server anymore.\n",
					id.String(), name.String(), object->Type().String(),
					object->Version());
				object->SetStatus(SYNC_STATUS_SERVER_REMOVED);
			} else {
				// the assumption is that we're running as client
				// and that means we should only have published objects,
				// so we mark this object for removal
				// TODO: instead of assumptions like these, use explicit flags to
				//		trigger a certain behaviour
fStatusOutput->PrintInfoMessage("marking published object 'removed': %s (%s)\n",
	id.String(), name.String());
				LOG_DEBUG("_HandleObsoleteObjects(): marking published object "
					"'removed': %s (%s)\n", id.String(), name.String());
				object->SetStatus(SYNC_STATUS_SERVER_REMOVED);
			}
			_ObjectsChanged();
		}
	}
}


// _ObjectsChanged
void
Synchronizer::_ObjectsChanged()
{
	if (!fObjectsChanged) {
		fObjectsChanged = true;
		Looper()->PostMessage(MSG_SYNCHRONIZATION_STARTED);
	}
}

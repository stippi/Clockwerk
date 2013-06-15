/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SYNCHRONIZER_H
#define SYNCHRONIZER_H

#include <Handler.h>
#include <String.h>

#include "HashMap.h"
#include "HashSet.h"
#include "StatusOutput.h"
#include "HashString.h"

class JobConnection;
class ServerObject;
class ServerObjectManager;
class ServerObjectFactory;

enum {
	MSG_SYNCHRONIZE						= 'sync',
	MSG_CANCEL							= 'cncl',

	MSG_REVISION_RESULT					= 'rvrs',
	MSG_QUERY_RESULT					= 'qurs',
	MSG_OBJECT_DOWNLOADED				= 'objd',

	MSG_SYNCHRONIZATION_STARTED			= 'sncs',
	MSG_SYNCHRONIZED					= 'sncd',
};

class Synchronizer : public BHandler {
public:
	class ObjectFilter {
	public:
								ObjectFilter();
		virtual					~ObjectFilter();

		virtual	bool			WantsObject(const BString& type,
									const BString& name, int32 version);
		virtual	void			PrepareObject(ServerObject* object);
		virtual	bool			WantsDependencies(
									const ServerObject* object);
	};

								Synchronizer(JobConnection* connection,
									ServerObjectManager* manager,
									ServerObjectFactory* factory,
									const BString& downloadScope,
									bool downloadAllObjects = true,
									bool testMode = false,
									StatusOutput* statusOutput = NULL);
	virtual						~Synchronizer();

	// BHandler interface
	virtual	void				MessageReceived(BMessage* message);

	// Synchronizer
			status_t			Init();
			void				SetObjectFilter(ObjectFilter* filter);
			void				SetConsumeLessCPU(bool consumeLess);

			status_t			Update(int64 clientLatestRevision = 0,
									int64 clientLocalRevision = 0);
			bool				InProgress() const
									{ return fInProgress; }

			status_t			Cancel();

			HashSet<HashString>& FinalMissingObjects()
									{ return fFinalMissingIDs; }
			HashSet<HashString>& ExistingButMissingOnServerObjects()
									{ return fLocallyExistingMissingOnServerIDs; }

			int64				ServerLatestRevision() const
									{ return fServerLatestRevision; }
			int64				ServerLocalRevision() const
									{ return fServerLocalRevision; }

			void				CleanUp();

private:
			status_t			_QueryRevision(int64 clientRevision);
			status_t			_HandleRevisionResult(BMessage* message);
			status_t			_QueryObjects();
			status_t			_HandleQueryResult(BMessage* message);
			status_t			_FetchNextMissingObject();
			status_t			_FindReferencedObjects(
									const ServerObject* object);
			status_t			_InsertDownloadedObject(BMessage* message,
									ServerObject** _object);			
			status_t			_HandleDownloaded(BMessage* message);
			void				_AllDownloadsDone();

			status_t			_HandleCancel(BMessage* message);

			void				_JobDeleted(const BString& serverID);

			void				_HandleObsoleteObjects();

			void				_ObjectsChanged();

	class MyDownloadJob;
	friend class MyDownloadJob;
	class CancelJob;
	friend class CancelJob;

	JobConnection*				fConnection;
	ServerObjectManager*		fManager;
	ServerObjectFactory*		fFactory;
	ObjectFilter*				fObjectFilter;
	BString						fDownloadScope;
	uint32						fNextChangeSetID;

	int64						fClientLatestRevision;
	int64						fClientLocalRevision;

	int64						fServerLatestRevision;
	int64						fServerLocalRevision;

	HashMap<HashString, int32>	fIDVersionMap;

	HashSet<HashString>			fMissingIDs;
	HashSet<HashString>			fOldIDs;
	HashSet<HashString>			fFinalMissingIDs;
	HashSet<HashString>			fLocallyExistingMissingOnServerIDs;
	int32						fOpenJobs;

	bool						fInProgress;
	bool						fCanceled;
	bool						fDownloadAllObjects;
	bool						fTestMode;
	bool						fConsumeLessCPU;

	bool						fObjectsChanged;
	bool						fErrorsOccured;
	bool						fFirstDownload;

	StatusOutput*				fStatusOutput;
	ConsoleStatusOutput			fConsoleOutput;
};

#endif // SYNCHRONIZER_H

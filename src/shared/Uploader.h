/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef UPLOADER_H
#define UPLOADER_H

#include <Handler.h>
#include <List.h>
#include <String.h>

#include "HashMap.h"
#include "HashSet.h"
#include "HashString.h"
#include "ReferencedObjectFinder.h"
#include "StatusOutput.h"

class JobConnection;
class ServerObject;
class ServerObjectManager;

enum {
	MSG_ALL_OBJECTS_UPLOADED	= 'aoul',
};

class Uploader : public BHandler {
public:
	// TODO: the UploadVisitor must be more powerful in order to
	// get the information about the dependencies, maybe the 
	// DepGraphNode struct needs to be accessible, since the information
	// is already inside those
	class UploadVisitor {
	public:
								UploadVisitor();
		virtual					~UploadVisitor();

		virtual	void			Visit(ServerObject* object, int32 level);
	};

								Uploader(JobConnection* connection,
									ServerObjectManager* manager,
									StatusOutput* statusOutput = NULL);
	virtual						~Uploader();

	// BHandler interface
	virtual	void				MessageReceived(BMessage* message);

	// Uploader
			status_t			Init();
			status_t			Collect();
			status_t			Upload();
			status_t			Cancel();
			void				CleanUp();

			status_t			VisitUploads(UploadVisitor* visitor) const;

private:
			status_t			_CollectObjectsForUpload();
			status_t			_StartNextGroupUpload();
			status_t			_HandleUploaded(BMessage* message);
			status_t			_HandleCancel(BMessage* message);

			void				_JobDeleted();
			void				_ReportFailure(status_t error);
			void				_Finished(status_t error);

	class MyGroupUploadJob;
	friend class MyGroupUploadJob;
	class CancelJob;
	friend class CancelJob;

	JobConnection*				fConnection;
	ServerObjectManager*		fManager;

	uint32						fNextChangeSetID;

	int32						fOpenJobs;

	// structures for managing the dependency graph
	struct DepGraphNode {
						DepGraphNode(ServerObject* object)
							: indegree(0)
							, successors(16)
							, object(object)
						{
						}

		int32			indegree;
		BList			successors;

		ServerObject*	object;
	};

	typedef HashMap<HashString, DepGraphNode*> IdNodeMap;

	IdNodeMap					fIdNodeMap;
	BList						fIndegreeZeroNodes;
	int32						fCurrentIndex;
	int32						fCurrentRetries;

			status_t			_FindReferencedObjects(DepGraphNode* node);
			status_t			_AddNodeToSortedList(DepGraphNode* node);
	//


	bool						fInProgress;
	bool						fCanceled;

	StatusOutput*				fStatusOutput;
	ConsoleStatusOutput			fConsoleOutput;
};

#endif // UPLOADER_H

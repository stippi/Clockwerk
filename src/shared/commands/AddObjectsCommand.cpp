/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "AddObjectsCommand.h"

#include <new>
#include <stdio.h>

#include <Entry.h>
#include <Node.h>

#include "common.h"

#include "Clip.h"
#include "CommonPropertyIDs.h"
#include "Selectable.h"
#include "Selection.h"
#include "ServerObjectManager.h"
#include "ServerObject.h"

using std::nothrow;

// constructor
AddObjectsCommand::AddObjectsCommand(ServerObjectManager* library,
									 ServerObject** const objects,
									 int32 count, Selection* selection)
	: Command(),
	  fLibrary(library),
	  fObjects((objects && count > 0) ? new (nothrow) ServerObject*[count]
	  								  : NULL),
	  fCount(count),

	  fObjectsAdded(false),

	  fSelection(selection)
{
	if (fObjects) {
		memcpy(fObjects, objects, fCount * sizeof(ServerObject*));
	}
}

// destructor
AddObjectsCommand::~AddObjectsCommand()
{
	if (!fObjectsAdded) {
		// the ServerObjects belong to us,
		// we might have marked the object "removed"
		// instead of having really removed it
		// to be 100% on the safe side, we check if the
		// object is still in the library
		for (int32 i = 0; i < fCount; i++) {
			if (!fLibrary->HasObject(fObjects[i]))
				fObjects[i]->Release();
		}
	}
	delete[] fObjects;
}

// InitCheck
status_t
AddObjectsCommand::InitCheck()
{
	if (fLibrary && fObjects && fCount > 0 && fObjects[0])
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
AddObjectsCommand::Perform()
{
	AutoWriteLocker _(fLibrary->Locker());

	// add objects
	for (int32 i = 0; i < fCount; i++) {
		if (!fObjects[i])
			continue;
		if (fObjects[i]->Status() == SYNC_STATUS_LOCAL_REMOVED) {
			fObjects[i]->SetStatus(SYNC_STATUS_MODIFIED);
		} else if (fObjects[i]->Status() == SYNC_STATUS_SERVER_REMOVED) {
			// ignore
		} else if (!fLibrary->AddObject(fObjects[i])) {
			// ERROR - roll back, remove the items
			// we already added
			printf("AddObjectsCommand::Perform() - "
				   "no memory to add items to playlist!\n");
			for (int32 j = i - 1; j >= 0; j--) {
				if (fObjects[j]->Status() == SYNC_STATUS_LOCAL)
					fLibrary->RemoveObject(fObjects[j]);
				else
					fObjects[i]->SetStatus(SYNC_STATUS_LOCAL_REMOVED);
			}
			return B_NO_MEMORY;
		}
		if (fSelection) {
			Selectable* selectable = dynamic_cast<Selectable*>(fObjects[i]);
			if (selectable) {
				fSelection->Select(selectable, i > 0);
			}
		}
	}
	fObjectsAdded = true;

	return B_OK;
}

// Undo
status_t
AddObjectsCommand::Undo()
{
	AutoWriteLocker _(fLibrary->Locker());

	// remove objects
	for (int32 i = fCount - 1; i >= 0; i--) {
		if (!fObjects[i])
			continue;
		if (fSelection) {
			Selectable* selectable = dynamic_cast<Selectable*>(fObjects[i]);
			if (selectable)
				fSelection->Deselect(selectable);
		}
		// local objects can simply be removed,
		// already published objects need to be marked
		// removed
		if (fObjects[i]->Status() == SYNC_STATUS_LOCAL) {
			fLibrary->RemoveObject(fObjects[i]);
			// make sure the object "file" is no longer
			// read into the library when Clockwerk starts
			// the next time
			// NOTE: in case the command is undone, then
			// the object will be in the library again and the
			// file is properly restored when Clockwerk exits
			_RemoveObjectFile(fObjects[i]->ID());
		} else {
			fObjects[i]->SetStatus(SYNC_STATUS_LOCAL_REMOVED);
		}
	}
	fObjectsAdded = false;

	return B_OK;
}

// GetName
void
AddObjectsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Add Objects";
	else {
		name << "Add " << fObjects[0]->Name();
	}
}

// #pragma mark -

// _RemoveObjectFile
void
AddObjectsCommand::_RemoveObjectFile(const BString& id)
{
	entry_ref ref;
	if (fLibrary->GetRef(id, ref) < B_OK)
		return;

	BNode node(&ref);
	if (node.InitCheck() < B_OK)
		return;

	node.RemoveAttr(kTypeAttr);
}


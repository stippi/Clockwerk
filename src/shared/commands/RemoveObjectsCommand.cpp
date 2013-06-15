/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RemoveObjectsCommand.h"

#include <new>
#include <stdio.h>

#include <Entry.h>
#include <Node.h>

#include "common.h"

#include "CommonPropertyIDs.h"
#include "Selectable.h"
#include "Selection.h"
#include "ServerObjectManager.h"
#include "ServerObject.h"

using std::nothrow;

// constructor
RemoveObjectsCommand::RemoveObjectsCommand(
							ServerObjectManager* library,
							ServerObject** const objects,
							int32 count, Selection* selection,
							bool remove)
	: Command(),
	  fLibrary(library),
	  fObjects((objects && count > 0) ? new (nothrow) ServerObject*[count]
	  								  : NULL),
	  fIndices(count > 0 ? new int32[count] : NULL),
	  fCount(count),

	  fObjectsRemoved(!remove),
	  fRemove(remove),

	  fSelection(selection)
{
	if (!fObjects)
		return;

	memcpy(fObjects, objects, fCount * sizeof(ServerObject*));

	if (!fLibrary || !fIndices)
		return;

	// NOTE: if the objects are not yet in the library,
	// indices will be -1, but that's ok, it will just
	// add them at the end
	for (int32 i = 0; i < fCount; i++) {
		fIndices[i] = fLibrary->IndexOf(fObjects[i]);
		if (fObjects[i])
			fObjects[i]->Acquire();
	}
}

// destructor
RemoveObjectsCommand::~RemoveObjectsCommand()
{
	if (fObjectsRemoved && fLibrary->ReadLock()) {
		// the ServerObjects belong to us,
		// we might have marked an object "removed"
		// instead of having really removed it
		// to be 100% on the safe side, we check if the
		// object is still in the library
		for (int32 i = 0; i < fCount; i++) {
			if (fObjects[i]) {
				if (!fLibrary->HasObject(fObjects[i])) {
					if (fObjects[i]->Status() == SYNC_STATUS_LOCAL)
						_RemoveObjectFile(fObjects[i]->ID(), true);
					// release once because we removed the object
					// from the library
					fObjects[i]->Release();
				}
				// release a second time because we aquired our own
				// reference
				fObjects[i]->Release();
			}
		}
		fLibrary->ReadUnlock();
	} else {
		for (int32 i = 0; i < fCount; i++) {
			if (fObjects[i])
				fObjects[i]->Release();
		}
	}

	delete[] fObjects;
	delete[] fIndices;
}

// InitCheck
status_t
RemoveObjectsCommand::InitCheck()
{
	if (fLibrary && fObjects && fIndices && fCount > 0 && fObjects[0])
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
RemoveObjectsCommand::Perform()
{
	if (fObjectsRemoved)
		return _UnRemoveObjects();
	else
		return _RemoveObjects();
}

// Undo
status_t
RemoveObjectsCommand::Undo()
{
	return Perform();
}

// GetName
void
RemoveObjectsCommand::GetName(BString& name)
{
	if (fRemove)
		name << "Remove ";
	else
		name << "Un-Remove ";

	if (fCount > 1)
		name << "Objects";
	else {
		name << fObjects[0]->Name();
	}
}

// #pragma mark -

// _RemoveObjects
status_t
RemoveObjectsCommand::_RemoveObjects()
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
		} else if (fObjects[i]->Status() == SYNC_STATUS_SERVER_REMOVED) {
			// ignore
		} else {
			fObjects[i]->SetStatus(SYNC_STATUS_LOCAL_REMOVED);
		}
	}
	fObjectsRemoved = true;

	return B_OK;
}

// _UnRemoveObjects
status_t
RemoveObjectsCommand::_UnRemoveObjects()
{
	AutoWriteLocker _(fLibrary->Locker());

	// add objects
	for (int32 i = 0; i < fCount; i++) {
		if (!fObjects[i])
			continue;
		if (fObjects[i]->Status() == SYNC_STATUS_LOCAL_REMOVED) {
			fObjects[i]->SetStatus(SYNC_STATUS_MODIFIED);
		} else {
			if (!fLibrary->AddObject(fObjects[i], fIndices[i])) {
				// ERROR - roll back, remove the items
				// we already added
				printf("RemoveObjectsCommand::Perform() - "
					   "no memory to add items to playlist!\n");
				for (int32 j = i - 1; j >= 0; j--) {
					if (fObjects[j]->Status() == SYNC_STATUS_LOCAL)
						fLibrary->RemoveObject(fObjects[j]);
					else
						fObjects[i]->SetStatus(SYNC_STATUS_LOCAL_REMOVED);
				}
				return B_NO_MEMORY;
			}
		}
		if (fSelection) {
			Selectable* selectable = dynamic_cast<Selectable*>(fObjects[i]);
			if (selectable)
				fSelection->Select(selectable, i > 0);
		}
	}
	fObjectsRemoved = false;

	return B_OK;
}

// _RemoveObjectFile
void
RemoveObjectsCommand::_RemoveObjectFile(const BString& id, bool forReal)
{
	entry_ref ref;
	if (fLibrary->GetRef(id, ref) < B_OK)
		return;

	if (forReal) {
		BEntry entry(&ref);
		if (entry.InitCheck() < B_OK)
			return;
	
		entry.Remove();
	} else {
		BNode node(&ref);
		if (node.InitCheck() < B_OK)
			return;
	
		node.RemoveAttr(kTypeAttr);
	}
}



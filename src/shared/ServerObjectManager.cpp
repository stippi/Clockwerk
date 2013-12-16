/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ServerObjectManager.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>
#include <File.h>
#include <InterfaceDefs.h>

#include "common.h"

#include "CommonPropertyIDs.h"
#include "OptionProperty.h"
#include "ProgressReporter.h"
#include "ServerObject.h"
#include "ServerObjectFactory.h"

int64	ServerObjectManager::sBaseID(real_time_clock_usecs());
vint32	ServerObjectManager::sNextID(0);
BString ServerObjectManager::sClientID("client0001");


// constructor
SOMListener::SOMListener()
{
}

// destructor
SOMListener::~SOMListener()
{
}

//	#pragma mark -


// constructor
AsyncSOMListener::AsyncSOMListener(BHandler* handler)
	: SOMListener()
	, AbstractLOAdapter(handler)
{
}

// destructor
AsyncSOMListener::~AsyncSOMListener()
{
}

// ObjectAdded
void
AsyncSOMListener::ObjectAdded(ServerObject* object, int32 index)
{
	BMessage message(MSG_OBJECT_ADDED);
	message.AddPointer("object", (void*)object);
	message.AddInt32("index", index);
	DeliverMessage(message);
}

// ObjectRemoved
void
AsyncSOMListener::ObjectRemoved(ServerObject* object)
{
	BMessage message(MSG_OBJECT_REMOVED);
	message.AddPointer("object", (void*)object);
	DeliverMessage(message);
}

//	#pragma mark -


// constructor
ServerObjectManager::ServerObjectManager()
	: fLocker(NULL)
	, fDirectory("")
	, fObjects(128)
	, fIDObjectMap()
	, fListeners(16)
	, fStateNeedsSaving(false)
	, fLoadRemovedObjects(true)
{
}

// destructor
ServerObjectManager::~ServerObjectManager()
{
	int32 count = fListeners.CountItems();
	if (count > 0) {
		debugger("~ServerObjectManager() - there are still "
				 "listeners attached\n");
	}
	_MakeEmpty();
}

//	#pragma mark -

// SetLocker
void
ServerObjectManager::SetLocker(RWLocker* locker)
{
	fLocker = locker;
}

// ReadLock
bool
ServerObjectManager::ReadLock()
{
	return fLocker->ReadLock();
}

// ReadLockWithTimeout
status_t
ServerObjectManager::ReadLockWithTimeout(bigtime_t timeout)
{
	return fLocker->ReadLockWithTimeout(timeout);
}

// ReadUnlock
void
ServerObjectManager::ReadUnlock()
{
	fLocker->ReadUnlock();
}

// IsReadLocked
bool
ServerObjectManager::IsReadLocked() const
{
	return fLocker->IsReadLocked();
}

// WriteLock
bool
ServerObjectManager::WriteLock()
{
	return fLocker->WriteLock();
}

// WriteLockWithTimeout
status_t
ServerObjectManager::WriteLockWithTimeout(bigtime_t timeout)
{
	return fLocker->WriteLockWithTimeout(timeout);
}

// WriteUnlock
void
ServerObjectManager::WriteUnlock()
{
	fLocker->WriteUnlock();
}

// IsWriteLocked
bool
ServerObjectManager::IsWriteLocked() const
{
	return fLocker->IsWriteLocked();
}

// #pragma mark -

// AddObject
bool
ServerObjectManager::AddObject(ServerObject* object)
{
	return AddObject(object, CountObjects());
}

// AddObject
bool
ServerObjectManager::AddObject(ServerObject* object, int32 index)
{
	if (!object)
		return false;

	if (index < 0 || index > CountObjects())
		index = CountObjects();

	status_t status = fIDObjectMap.Put(object->ID().String(), object);
	if (status < B_OK)
		return false;

	bool success = fObjects.AddItem((void*)object, index);
	if (success) {
		object->AttachedToManager(this);
		_NotifyObjectAdded(object, index);
		_StateChanged();
	} else {
		fIDObjectMap.Remove(object->ID().String());
		fprintf(stderr, "ServerObjectManager::AddObject() - out of memory!\n");
	}

	return success;
}

// RemoveObject
bool
ServerObjectManager::RemoveObject(ServerObject* object)
{
	bool success = fObjects.RemoveItem((void*)object);
	if (success) {
		object->DetachedFromManager();
		fIDObjectMap.Remove(object->ID().String());
		_NotifyObjectRemoved(object);
// NOTE only needed for xml based object manager, deprecated
//		_StateChanged();
	}

	return success;
}

// RemoveObject
ServerObject*
ServerObjectManager::RemoveObject(int32 index)
{
	ServerObject* object = (ServerObject*)fObjects.RemoveItem(index);
	if (object) {
		object->DetachedFromManager();
		fIDObjectMap.Remove(object->ID().String());
		_NotifyObjectRemoved(object);
// NOTE only needed for xml based object manager, deprecated
//		_StateChanged();
	}

	return object;
}

// #pragma mark -

// CountObjects
int32
ServerObjectManager::CountObjects() const
{
	return fObjects.CountItems();
}

// HasObject
bool
ServerObjectManager::HasObject(ServerObject* object) const
{
	if (!object)
		return false;
//	return fObjects.HasItem((void*)object);
	return fIDObjectMap.ContainsKey(object->ID().String());
}

// IndexOf
int32
ServerObjectManager::IndexOf(ServerObject* object) const
{
	return fObjects.IndexOf((void*)object);
}

// #pragma mark -

// MakeEmpty
void
ServerObjectManager::MakeEmpty()
{
	_MakeEmpty();
	_StateChanged();
}

// #pragma mark -

// ObjectAt
ServerObject*
ServerObjectManager::ObjectAt(int32 index) const
{
	return (ServerObject*)fObjects.ItemAt(index);
}

// ObjectAtFast
ServerObject*
ServerObjectManager::ObjectAtFast(int32 index) const
{
	return (ServerObject*)fObjects.ItemAtFast(index);
}

// #pragma mark -

// FindObject
ServerObject*
ServerObjectManager::FindObject(const BString& serverID) const
{
	if (!fIDObjectMap.ContainsKey(serverID.String()))
		return NULL;

	return fIDObjectMap.Get(serverID.String()).value;

//	int32 count = CountObjects();
//	for (int32 i = 0; i < count; i++) {
//		ServerObject* object = ObjectAtFast(i);
//		if (object->ID() == serverID) {
//			return object;
//		}
//	}
//
////	fprintf(stderr, "ServerObjectManager::FindObject() - "
////					"didn't find object by id %lld\n", id);
//	return NULL;
}

// IDChanged
status_t
ServerObjectManager::IDChanged(ServerObject* object,
	const BString& oldID)
{
printf("ServerObjectManager::IDChanged(%s)\n", object->Name().String());
	fIDObjectMap.Remove(oldID.String());
	return fIDObjectMap.Put(object->ID().String(), object);
}

// ResolveDependencies
status_t
ServerObjectManager::ResolveDependencies(ProgressReporter* reporter) const
{
	if (reporter)
		reporter->SetProgressTitle("Resolving dependencies"B_UTF8_ELLIPSIS);

	// first, make sure that the call is used on every
	// object
	int32 count = CountObjects();
	for (int32 i = 0; i < count; i++)
		ObjectAtFast(i)->SetDependenciesResolved(false);

	// now resolve the dependencies, but due to it's
	// sometimes being recursive, we can check if an object
	// is already valid
	int32 progressStep = max_c(1, count / 500);
	for (int32 i = 0; i < count; i++) {
		if (reporter && i % progressStep == 0)
			reporter->ReportProgress((i + 1) * 100.0 / count);

		if (ObjectAtFast(i)->IsValid())
			continue;
		status_t ret = ObjectAtFast(i)->ResolveDependencies(this);
		ObjectAtFast(i)->SetDependenciesResolved(ret == B_OK);
	}

	return B_OK;
}

// UpdateVersion
void
ServerObjectManager::UpdateVersion(const BString& serverID, int32 version)
{
	int32 count = CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = ObjectAtFast(i);
		if (object->ID() == serverID) {
printf("found object %s, setting published (version: %ld)\n",
serverID.String(), version);
			object->SetPublished(version);
			_StateChanged();
			break;
		}
	}
}

// SetClientID
void
ServerObjectManager::SetClientID(const char* clientID)
{
	// NOTE: no locking, this is assumed to be called
	// once on program start
	sClientID = clientID;
}

// NextID
BString
ServerObjectManager::NextID()
{
	BString id(sClientID);
		// TODO: get the "client name" from somewhere

	id << ":" << sBaseID << ":" << atomic_add(&sNextID, 1);
		// TODO: theoretically, we would have to watch
		// out for overflow of fNextID (and reinit sBaseID)

	return id;
}

// ForceStateChanged
void
ServerObjectManager::ForceStateChanged()
{
	AutoWriteLocker locker(fLocker);
	if (!locker.IsLocked())
		return;

	_StateChanged();
}

// StateChanged
void
ServerObjectManager::StateChanged()
{
	// empty
}

// SetIgnoreStateChanges
void
ServerObjectManager::SetIgnoreStateChanges(bool ignore)
{
	// empty
}

// FlushObjects
status_t
ServerObjectManager::FlushObjects(ServerObjectFactory* factory)
{
	status_t ret = B_OK;

	int32 count = CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = ObjectAtFast(i);
		if (object->IsMetaDataOnly() || object->IsDataSaved())
			continue;

		status_t status = factory->StoreObject(object, this);
		if (status < B_OK) {
			ret = status;
			if (status == B_NO_MEMORY)
				break;
		}
		object->SetDataSaved(true);
	}

	return ret;
}

// IsStateSaved
bool
ServerObjectManager::IsStateSaved() const
{
	return !fStateNeedsSaving;
}

// StateSaved
void
ServerObjectManager::StateSaved()
{
	fStateNeedsSaving = false;
}

// SetDirectory
void
ServerObjectManager::SetDirectory(const char* directory)
{
	if (fDirectory.Length() != 0 && fDirectory != directory) {
		print_error("ServerObjectManager::SetDirectory() - "
			"cannot switch library path at runtime (%s -> %s), ignoring!\n",
			fDirectory.String(), directory);
		return;
	}
	fDirectory = directory;
}


// GetRef
status_t
ServerObjectManager::GetRef(const BString& id, entry_ref& ref)
{
	return B_ERROR;
}

// GetRef
status_t
ServerObjectManager::GetRef(const ServerObject* object, entry_ref& ref)
{
	return B_ERROR;
}

// #pragma mark -

// SetLoadRemovedObjects
void
ServerObjectManager::SetLoadRemovedObjects(bool load)
{
	fLoadRemovedObjects = load;
}

// #pragma mark -

// AddListener
bool
ServerObjectManager::AddListener(SOMListener* listener)
{
	AutoWriteLocker locker(fLocker);
	if (!locker.IsLocked())
		return false;

	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem(listener);
	return false;
}

// RemoveListener
bool
ServerObjectManager::RemoveListener(SOMListener* listener)
{
	AutoWriteLocker locker(fLocker);
	if (!locker.IsLocked())
		return false;

	return fListeners.RemoveItem(listener);
}

// #pragma mark -

// _StateChanged
void
ServerObjectManager::_StateChanged()
{
	fStateNeedsSaving = true;
	// TODO/NOTE: it would be more efficient to be
	// able to suspend state changes and re-enable
	// them at the end of a larger "transaction",
	// but it would be less "safe" at the same time
	StateChanged();
}

// _MakeEmpty
void
ServerObjectManager::_MakeEmpty()
{
	fIDObjectMap.Clear();

	int32 count = CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = ObjectAtFast(i);
		object->DetachedFromManager();
		_NotifyObjectRemoved(object);
		object->Release();
	}
	fObjects.MakeEmpty();
}

// #pragma mark -

// _NotifyObjectAdded
void
ServerObjectManager::_NotifyObjectAdded(ServerObject* object,
										int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		SOMListener* listener = (SOMListener*)listeners.ItemAtFast(i);
		listener->ObjectAdded(object, index);
	}
}

// _NotifyObjectRemoved
void
ServerObjectManager::_NotifyObjectRemoved(ServerObject* object) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		SOMListener* listener = (SOMListener*)listeners.ItemAtFast(i);
		listener->ObjectRemoved(object);
	}
}


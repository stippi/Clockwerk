/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <stdlib.h>
#include <string.h>

#include "IDManager.h"

// constructor
IDManager::IDManager(int32 initialCapacity)
	: fObjectToID(initialCapacity),
	  fIDToObject(initialCapacity),
	  fNextID(0)
{
}

// destructor
IDManager::~IDManager()
{
	for (int32 i = 0; Entry* entry = (Entry*)fObjectToID.ItemAt(i); i++)
		delete entry;
}

// AssociateObjectWithID
void
IDManager::AssociateObjectWithID(const void* object, const char* id)
{
	_AssociateObjectWithID(object, id);
}

// GetIDForObject
const char*
IDManager::GetIDForObject(const void* object)
{
	const char* id = NULL;
	if (object) {
		int32 objectIndex = _FindObjectInsertionIndex(object);
		if (objectIndex >= 0) {
			Entry* entry = _ObjectEntryAt(objectIndex);
			if (entry && entry->object == object)
				id = entry->id.String();
			else {
				// not found: create and add a new entry
				BString newID;
				_GenerateNewID(newID);
				entry = _AssociateObjectWithID(object, newID.String());
				id = entry->id.String();
			}
		}
	}
	return id;
}

// GetObjectForID
void*
IDManager::GetObjectForID(const char* id) const
{
	void* object = NULL;
	if (id) {
		int32 idIndex = _FindIDInsertionIndex(id);
		if (idIndex >= 0) {
			Entry* entry = _IDEntryAt(idIndex);
			if (entry && entry->id.Compare(id) == 0)
				object = entry->object;
		}
	}
	return object;
}

// RemoveObject
bool
IDManager::RemoveObject(const void* object)
{
	bool found = false;
	if (const char* id = GetIDForObject(object)) {
		found = true;
		_RemoveEntry(object, id);
	}
	return found;
}

// RemoveID
bool
IDManager::RemoveID(const char* id)
{
	bool found = false;
	if (const void* object = GetObjectForID(id)) {
		found = true;
		_RemoveEntry(object, id);
	}
	return found;
}

// _AssociateObjectWithID
IDManager::Entry*
IDManager::_AssociateObjectWithID(const void* object, const char* id)
{
	Entry* entry = NULL;
	if (object && id) {
		// Clone the ID to avoid problems, if it belongs to one of the entries
		// we delete.
		BString clonedID(id);
		id = clonedID.String();
		RemoveObject(object);
		RemoveID(id);
		int32 objectIndex = _FindObjectInsertionIndex(object);
		int32 idIndex = _FindIDInsertionIndex(id);
		if (objectIndex >= 0 && idIndex >= 0) {
			entry = new Entry;
			entry->object = const_cast<void*>(object);
			fObjectToID.AddItem(entry, objectIndex);
			fIDToObject.AddItem(entry, idIndex);
			// check next ID
			int32 intID = atol(id);
			if (intID >= fNextID)
				fNextID = intID + 1;
		}
	}
	return entry;
}

// _RemoveEntry
void
IDManager::_RemoveEntry(const void* object, const char* id)
{
	int32 objectIndex = _FindObjectInsertionIndex(object);
	int32 idIndex = _FindIDInsertionIndex(id);
	Entry* entry = _ObjectEntryAt(objectIndex);
	fObjectToID.RemoveItem(objectIndex);
	fIDToObject.RemoveItem(idIndex);
	delete entry;
}

// _CountEntries
int32
IDManager::_CountEntries() const
{
	return fObjectToID.CountItems();
}

// _IDEntryAt
IDManager::Entry*
IDManager::_IDEntryAt(int32 index) const
{
	return (Entry*)fIDToObject.ItemAt(index);
}

// _ObjectEntryAt
IDManager::Entry*
IDManager::_ObjectEntryAt(int32 index) const
{
	return (Entry*)fObjectToID.ItemAt(index);
}

// _FindObjectInsertionIndex
int32
IDManager::_FindObjectInsertionIndex(const void* object) const
{
	int32 index = -1;
	if (object) {
		// binary search
		int32 lower = 0;
		int32 upper = _CountEntries();
		while (lower < upper) {
			int32 mid = (lower + upper) / 2;
			const void* midObject = _ObjectEntryAt(mid)->object;
			if (midObject == object)
				upper = mid;
			else
				lower = mid + 1;
		}
		index = lower;
	}
	return index;
}

// _FindIDInsertionIndex
int32
IDManager::_FindIDInsertionIndex(const char* id) const
{
	int32 index = -1;
	if (id) {
		// binary search
		int32 lower = 0;
		int32 upper = _CountEntries();
		while (lower < upper) {
			int32 mid = (lower + upper) / 2;
			const char* midID = _IDEntryAt(mid)->id.String();
			if (strcmp(midID, id) >= 0)
				upper = mid;
			else
				lower = mid + 1;
		}
		index = lower;
	}
	return index;
}

// _GenerateNewID
void
IDManager::_GenerateNewID(BString& id)
{
	id = "";
	id << fNextID;
}


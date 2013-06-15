/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "AttributeServerObjectManager.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fs_attr.h>

#include <ByteOrder.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <InterfaceDefs.h>
#include <NodeInfo.h>
#include <Path.h>

#include "common.h"

#include "CommonPropertyIDs.h"
#include "Debug.h"
#include "ProgressReporter.h"
#include "Property.h"
#include "ServerObject.h"
#include "ServerObjectFactory.h"

// constructor
AttributeServerObjectManager::AttributeServerObjectManager()
	: ServerObjectManager()
	, fIngoreStateChanges(false)
{
}

// destructor
AttributeServerObjectManager::~AttributeServerObjectManager()
{
}

// #pragma mark -

// Init
status_t
AttributeServerObjectManager::Init(const char* directory,
	ServerObjectFactory* factory, bool resolvDependencies,
	ProgressReporter* reporter)
{
	if (!directory || !factory)
		return B_BAD_VALUE;

	if (reporter)
		reporter->SetProgressTitle("Loading object library"B_UTF8_ELLIPSIS);

	SetDirectory(directory);

	// lock the directory
	BDirectory dir;
	while (dir.SetTo(directory) == B_BUSY) {
		printf("AttributeServerObjectManager::Init() - "
			   "unable to acquire directory node... retrying\n");
		snooze(100000);
	}
//	while (dir.Lock() == B_BUSY) {
//		printf("AttributeServerObjectManager::Init() - "
//			   "unable to lock directory node... retrying\n");
//		snooze(100000);
//	}

	status_t ret = dir.InitCheck();
	if (ret < B_OK) {
		printf("AttributeServerObjectManager::Init() - "
			   "unable to init directory: %s\n", strerror(ret));
		return ret;
	}

	fIngoreStateChanges = true;
		// don't save state during loading it

	int32 entryCount = 0;
	int32 currentEntryIndex = 1;

	BEntry entry;
	while (reporter && dir.GetNextEntry(&entry, false) == B_OK)
		entryCount++;

	bool loadRemovedObjects = LoadRemovedObjects();

	dir.Rewind();
	while (dir.GetNextEntry(&entry, false) == B_OK) {
		BNode node(&entry);
		if (node.InitCheck() < B_OK)
			continue;

		// ignore this object if configured so and if this
		// object has "removed" status
		if (!loadRemovedObjects) {
			BString status;
			if (node.ReadAttrString(kStatusAttr, &status) == B_OK
				&& status == "Removed") {
				continue;
			}
		}

		char name[B_FILE_NAME_LENGTH];
		entry.GetName(name);
		BString serverID(name);

		ret = _CreateObjectFromNode(node, serverID, factory);
		if (ret < B_OK)
			break;

		if (reporter) {
			currentEntryIndex++;
			reporter->ReportProgress(
				(float)currentEntryIndex * 100.0f / (float)entryCount);
		}
	}

	fIngoreStateChanges = false;

	if (ret == B_OK && resolvDependencies)
		ret = ResolveDependencies(reporter);

	StateSaved();

	RETURN_ERROR(ret);
}

// StateChanged
void
AttributeServerObjectManager::StateChanged()
{
	if (fIngoreStateChanges) {
		// we ignore the call, but make sure that we
		// remember we need to save
		if (ServerObjectManager::IsStateSaved())
			ForceStateChanged();
		return;
	}

	if (BString(Directory()).Length() == 0) {
		print_error("AttributeServerObjectManager::StateChanged() - "
			"no object directory specified!\n");
		return;
	}

	BDirectory dir;
	while (dir.SetTo(Directory()) == B_BUSY) {
		printf("AttributeServerObjectManager::StateChanged() - "
			   "unable to acquire directory node... retrying\n");
		snooze(100000);
	}

	status_t ret = dir.InitCheck();
	if (ret < B_OK) {
		printf("AttributeServerObjectManager::StateChanged() - "
			   "unable to create directory: %s\n", strerror(ret));
		return;
	}

	// iterate over objects and store them in node attributes
	int32 count = CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = ObjectAtFast(i);
		if (object->IsMetaDataSaved()) {
			// this object does not need saving
//printf("not saving object %s, because it is up to date\n", object->Name().String());
			continue;
		}
//print_info("saving attributes of object: %s (%s), version: %ld\n",
//	object->Name().String(), object->Type().String(), object->Version());
		if (_CreateNodeFromObject(dir, object) < B_OK) {
			print_error("AttributeServerObjectManager::StateChanged() - "
				" stopped at %ld\n", i);
			break;
		}
		object->SetMetaDataSaved(true);
	}

	// flush disk cache
	sync();

	StateSaved();
}

// SetIgnoreStateChanges
void
AttributeServerObjectManager::SetIgnoreStateChanges(bool ignore)
{
	fIngoreStateChanges = ignore;
}

// IsStateSaved
bool
AttributeServerObjectManager::IsStateSaved() const
{
	// iterate over objects and store them in node attributes
	int32 count = CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = ObjectAtFast(i);
		if (!object->IsMetaDataSaved() || !object->IsDataSaved())
			return false;
	}
	return ServerObjectManager::IsStateSaved();
}

// GetRef
status_t
AttributeServerObjectManager::GetRef(const BString& id, entry_ref& ref)
{
	BPath path;
	status_t error = _GetPath(id, path);
	if (error < B_OK)
		return error;
	return get_ref_for_path(path.Path(), &ref);
}

// GetRef
status_t
AttributeServerObjectManager::GetRef(const ServerObject* object, entry_ref& ref)
{
	return GetRef(object->ID(), ref);
}

// #pragma mark -

// _GetPath
status_t
AttributeServerObjectManager::_GetPath(const BString& id, BPath& path)
{
	if (id.Length() <= 0)
		return B_BAD_VALUE;
	return path.SetTo(Directory(), id.String());
}

// _CreateObjectFromNode
status_t
AttributeServerObjectManager::_CreateObjectFromNode(BNode& node,
	const BString& serverID, ServerObjectFactory* factory)
{
	// NOTE: only fail in case of B_NO_MEMORY

	attr_info info;
	if (node.GetAttrInfo(kTypeAttr, &info) < B_OK)
		return B_OK;

	char type[info.size + 1];
	if (node.ReadAttr(kTypeAttr, info.type, 0, type, info.size) < B_OK) {
		printf("failed to read type attribute from object \"%s\"\n",
			   serverID.String());
		return B_OK;
	}

	type[info.size] = 0;
	BString typeString(type);

	// have the factory instantiate the object for the type
	ServerObject* object = factory->Instantiate(typeString, serverID,
												this);
	if (!object)
		return B_NO_MEMORY;

	// restore object properties and add to list
	if (_RestorePropertiesFromNode(node, object) < B_OK ||
		!AddObject(object)) {
		delete object;
		return B_NO_MEMORY;
	}

	// this object is up to date
	object->SetMetaDataSaved(true);

	return B_OK;
}

// _CreateNodeFromObject
status_t
AttributeServerObjectManager::_CreateNodeFromObject(BDirectory& directory,
	const ServerObject* object) const
{
	// create the file
	BString id = object->ID();
	BFile file(&directory, id.String(), B_CREATE_FILE | B_WRITE_ONLY);

	status_t ret = file.InitCheck();
	if (ret < B_OK) {
		printf("_CreateNodeFromObject() - failed to create file "
			   "\"%s\": %s\n", id.String(), strerror(ret));
		return ret;
	}

	ret = _StorePropertiesInNode(file, object);
	if (ret == B_OK) {
		BNodeInfo nodeInfo(&file);
		if (nodeInfo.InitCheck() == B_OK)
			nodeInfo.SetType(kNativeDocumentMimeType);
	}
	return ret;
}

// _RestorePropertiesFromNode
status_t
AttributeServerObjectManager::_RestorePropertiesFromNode(BNode& node,
	ServerObject* object) const
{
	int32 count = object->CountProperties();
	for (int32 i = 0; i < count; i++) {
		// fetch property
		Property* property = object->PropertyAtFast(i);

		if (property->Identifier() == PROPERTY_TYPE
			|| property->Identifier() == PROPERTY_ID)
			// not interested in these
			// "type" has already been read, and the id is the
			// file name
			continue;

		// build attribute name from property identifier
		unsigned id = B_HOST_TO_BENDIAN_INT32(property->Identifier());
		char idString[5];
		sprintf(idString, "%.4s", (const char*)&id);
		idString[4] = 0;

		BString attrName(kAttrPrefix);
		attrName << idString;

		// get information about this attribute
		attr_info info;
		if (node.GetAttrInfo(attrName.String(), &info) < B_OK) {
//			printf("error getting attribute info for \"%s\"\n",
//				attrName.String());
			continue;
		}

		// read the attribute
		char buffer[info.size + 1];
		ssize_t read = node.ReadAttr(attrName.String(), info.type, 0, buffer,
			info.size);
		if (read < (ssize_t)info.size) {
			printf("error reading attribute \"%s\"\n", attrName.String());
			continue;
		}

		// set the property to the attribute string
		// make sure the string is zero terminated
		buffer[info.size] = 0;

		if (property->SetValue(buffer))
			object->ValueChanged(property);
	}

	return B_OK;
}

// _StorePropertiesInNode
status_t
AttributeServerObjectManager::_StorePropertiesInNode(BNode& node,
	const ServerObject* object) const
{
	int32 count = object->CountProperties();
	for (int32 i = 0; i < count; i++) {
		// fetch property
		Property* property = object->PropertyAtFast(i);

		if (property->Identifier() == PROPERTY_ID)
			// id is already stored in the file's name
			continue;

		// build attribute name from property identifier
		unsigned id = B_HOST_TO_BENDIAN_INT32(property->Identifier());
		char idString[5];
		sprintf(idString, "%.4s", (const char*)&id);
		idString[4] = 0;

		BString attrName(kAttrPrefix);
		attrName << idString;

		// get property value as string and write attribute
		BString value;
		property->GetValue(value);

		// write the terminating zero too, that avoid running
		// into a Tracker bug where truely empty attributes are
		// not copied
		size_t length = value.Length() + 1;
		ssize_t written = node.WriteAttr(attrName.String(),
			B_STRING_TYPE, 0, value.String(), length);
		if (written < (ssize_t)length) {
			status_t ret = B_ERROR;
			if (written < 0)
				ret = (status_t)written;
			printf("error writing attribue \"%s\": %s\n",
				   attrName.String(), strerror(ret));
			if (ret == B_NO_MEMORY || ret == B_DEVICE_FULL)
				return ret;
			continue;
		}
	}

	return B_OK;
}


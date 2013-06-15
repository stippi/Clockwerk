/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "XMLHelper.h"

#include <fstream>

#include <DataIO.h>
#include <File.h>
#include <Font.h>

#include "XMLStorable.h"

// static members
BLocker XMLHelper::fGlobalLock("XMLHelper lock");

// constructor
XMLHelper::XMLHelper()
	: IDManager(),
	  ParameterManager(),
	  fCurrentPath("")
{
}

// destructor
XMLHelper::~XMLHelper()
{
}

// Load
status_t
XMLHelper::Load(const char* filename)
{
	status_t error = B_OK;
	BFile file(filename, B_READ_ONLY);
	error = file.InitCheck();
	if (error == B_OK) {
		error = Load(file);
		if (error == B_OK)
			fCurrentPath.SetTo(filename);
	}
	return error;
}

// Load
// status_t
// XMLHelper::Load(BDataIO& input);

// Save
status_t
XMLHelper::Save(const char* filename)
{
	status_t error = B_OK;
	BFile file(filename, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	error = file.InitCheck();
	if (error == B_OK)
		error = Save(file);
	return error;
}

// Lock
bool
XMLHelper::Lock()
{
	return fGlobalLock.Lock();
}

// LockWithTimeout
status_t
XMLHelper::LockWithTimeout(bigtime_t timeout)
{
	return fGlobalLock.LockWithTimeout(timeout);
}

// Unlock
void
XMLHelper::Unlock()
{
	fGlobalLock.Unlock();
}

// IsLocked
bool
XMLHelper::IsLocked() const
{
	return fGlobalLock.IsLocked();
}


// Save
// status_t
// XMLHelper::Save(BDataIO& output);

// GetCurrentPath
const char*
XMLHelper::GetCurrentPath() const
{
	return fCurrentPath.String();
}

// CreateTag
// status_t
// XMLHelper::CreateTag(const char* tagname);

// OpenTag
// status_t
// XMLHelper::OpenTag();

// OpenTag
// status_t
// XMLHelper::OpenTag(const char* tagname);

// CloseTag
// status_t
// XMLHelper::CloseTag();

// RewindTag
// status_t
// XMLHelper::RewindTag();

// GetTagName
// status_t
// XMLHelper::GetTagName(BString& name) const;


// StoreObject
status_t
XMLHelper::StoreObject(const XMLStorable& object)
{
	return object.XMLStore(*this);
}

// StoreObject
status_t
XMLHelper::StoreObject(const XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = StoreObject(*object);
	else
		error = B_BAD_VALUE;
	return error;
}

// StoreObject
status_t
XMLHelper::StoreObject(const char* tagname, const XMLStorable& object)
{
	status_t error = B_OK;
	if (tagname) {
		error = CreateTag(tagname);
		if (error == B_OK) {
			error = StoreObject(object);
			status_t err = CloseTag();
			if (error == B_OK)
				error = err;
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// StoreObject
status_t
XMLHelper::StoreObject(const char* tagname, const XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = StoreObject(tagname, *object);
	else
		error = B_BAD_VALUE;
	return error;
}

// StoreIDObject
status_t
XMLHelper::StoreIDObject(const XMLStorable& object)
{
	status_t error = B_ERROR;
	if (const char* id = GetIDForObject(&object)) {
		error = SetAttribute("ID", id);
		if (error == B_OK)
			error = object.XMLStore(*this);
	}
	return error;
}

// StoreIDObject
status_t
XMLHelper::StoreIDObject(const XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = StoreIDObject(*object);
	else
		error = B_BAD_VALUE;
	return error;
}

// StoreIDObject
status_t
XMLHelper::StoreIDObject(const char* tagname, const XMLStorable& object)
{
	status_t error = B_OK;
	if (tagname) {
		error = CreateTag(tagname);
		if (error == B_OK) {
			error = StoreIDObject(object);
			status_t err = CloseTag();
			if (error == B_OK)
				error = err;
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// StoreIDObject
status_t
XMLHelper::StoreIDObject(const char* tagname, const XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = StoreIDObject(tagname, *object);
	else
		error = B_BAD_VALUE;
	return error;
}

// RestoreObject
status_t
XMLHelper::RestoreObject(XMLStorable& object)
{
	return object.XMLRestore(*this);
}

// RestoreObject
status_t
XMLHelper::RestoreObject(XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = RestoreObject(*object);
	else
		error = B_BAD_VALUE;
	return error;
}

// RestoreObject
status_t
XMLHelper::RestoreObject(const char* tagname, XMLStorable& object)
{
	status_t error = B_OK;
	if (tagname) {
		error = OpenTag(tagname);
		if (error == B_OK) {
			error = RestoreObject(object);
			status_t err = CloseTag();
			if (error == B_OK)
				error = err;
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// RestoreObject
status_t
XMLHelper::RestoreObject(const char* tagname, XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = RestoreObject(tagname, *object);
	else
		error = B_BAD_VALUE;
	return error;
}

// RestoreIDObject
status_t
XMLHelper::RestoreIDObject(XMLStorable& object)
{
	BString id;
	if (GetAttribute("ID", id) == B_OK)
		AssociateObjectWithID(&object, id.String());
	else
		GetIDForObject(&object);
	return object.XMLRestore(*this);
}

// RestoreIDObject
status_t
XMLHelper::RestoreIDObject(XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = RestoreIDObject(*object);
	else
		error = B_BAD_VALUE;
	return error;
}

// RestoreIDObject
status_t
XMLHelper::RestoreIDObject(const char* tagname, XMLStorable& object)
{
	status_t error = B_OK;
	if (tagname) {
		error = OpenTag(tagname);
		if (error == B_OK) {
			error = RestoreIDObject(object);
			status_t err = CloseTag();
			if (error == B_OK)
				error = err;
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// RestoreIDObject
status_t
XMLHelper::RestoreIDObject(const char* tagname, XMLStorable* object)
{
	status_t error = B_OK;
	if (object)
		error = RestoreIDObject(tagname, *object);
	else
		error = B_BAD_VALUE;
	return error;
}

// AssociateStorableWithID
void
XMLHelper::AssociateStorableWithID(XMLStorable* object, const char* id)
{
	AssociateObjectWithID(object, id);
}

// GetIDForStorable
const char*
XMLHelper::GetIDForStorable(XMLStorable* object)
{
	return GetIDForObject(object);
}

// GetStorableForID
XMLStorable*
XMLHelper::GetStorableForID(const char* id)
{
	return (XMLStorable*)GetObjectForID(id);
}

// RemoveStorable
bool
XMLHelper::RemoveStorable(XMLStorable* object)
{
	return RemoveObject(object);
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ServerObject.h"

#include <stdio.h>

#include <debugger.h>
#include <InterfaceDefs.h>
#include <Message.h>

#include "CommonPropertyIDs.h"
#include "Int64Property.h"
#include "OptionProperty.h"
#include "Property.h"
#include "PropertyObjectFactory.h"
#include "ServerObjectManager.h"

// constructor
ServerObject::ServerObject(const char* type)
	: PropertyObject()
	, Referencable()
	, fInitStatus(PropertyObjectFactory::InitPropertyObject(type, this))
	, fDependenciesResolved(false)
	, fMetaDataSaved(false)
	, fDataSaved(false)
	, fCachedIDProperty(NULL)
	, fCachedStatusProperty(NULL)
	, fObjectManager(NULL)
{
}

// constructor
ServerObject::ServerObject(const ServerObject& other, bool deep)
	: PropertyObject(other, deep)
	, Referencable()
	, fInitStatus(other.fInitStatus)
	, fDependenciesResolved(other.fDependenciesResolved)
	, fMetaDataSaved(false)
	, fDataSaved(false)
	, fCachedIDProperty(NULL)
	, fCachedStatusProperty(NULL)
	, fObjectManager(NULL)
{
}

// destructor
ServerObject::~ServerObject()
{
}

// SetTo
status_t
ServerObject::SetTo(const ServerObject* other)
{
	// called when a newer version of this
	// object has been downloaded
	fMetaDataSaved = false;
	fDataSaved = false;

	BMessage metaData;
	status_t ret = other->Archive(&metaData);
	if (ret == B_OK)
		ret = Unarchive(&metaData);
	return ret;
}

// ValueChanged
void
ServerObject::ValueChanged(Property* property)
{
	fMetaDataSaved = false;

	if (property->IsEditable() && Status() == SYNC_STATUS_PUBLISHED)
		SetStatus(SYNC_STATUS_MODIFIED);

	PropertyObject::ValueChanged(property);
}

// IsValid
bool
ServerObject::IsValid() const
{
	return fDependenciesResolved && fInitStatus == B_OK;
}

// SetDependenciesResolved
void
ServerObject::SetDependenciesResolved(bool resolved)
{
	fDependenciesResolved = resolved;
}

// SetName
void
ServerObject::SetName(const BString& name)
{
	SetValue(PROPERTY_NAME, name.String());
}

// Name
BString
ServerObject::Name() const
{
	BString name;
	GetValue(PROPERTY_NAME, name);
	return name;
}


// SetID
void
ServerObject::SetID(const BString& id)
{
	BString oldID = ID();
	SetValue(PROPERTY_ID, id.String());

	if (fObjectManager)
		fObjectManager->IDChanged(this, oldID);
}

// ID
BString
ServerObject::ID() const
{
	if (!fCachedIDProperty) {
		fCachedIDProperty = dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_ID));
	}
	if (!fCachedIDProperty)
		return BString("");
	return BString(fCachedIDProperty->Value());
}

// SetVersion
void
ServerObject::SetVersion(int32 version)
{
	SetValue(PROPERTY_VERSION, version);
}

// Version
int32
ServerObject::Version() const
{
	return Value(PROPERTY_VERSION, -1L);
}

// Type
BString
ServerObject::Type() const
{
	BString type;
	GetValue(PROPERTY_TYPE, type);
	return type;
}

// SetPublished
void
ServerObject::SetPublished(int32 version)
{
	SuspendNotifications(true);

	SetVersion(version);
	SetStatus(SYNC_STATUS_PUBLISHED);

	SuspendNotifications(false);
}

// IsMetaDataOnly
bool
ServerObject::IsMetaDataOnly() const
{
	return true;
}

// IsExternalData
bool
ServerObject::IsExternalData() const
{
	return false;
}

// SetStatus
void
ServerObject::SetStatus(int32 status)
{
	if (!fCachedStatusProperty) {
		fCachedStatusProperty = dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_SYNC_STATUS));
	}

	if (fCachedStatusProperty && fCachedStatusProperty->SetValue(status)) {
		fMetaDataSaved = false;
//switch (status) {
//case SYNC_STATUS_LOCAL:
//printf("ServerObject::SetStatus(LOCAL)\n");
//if (modifiers() & B_SHIFT_KEY)
//debugger("LOCAL");
//break;
//case SYNC_STATUS_PUBLISHED:
//printf("ServerObject::SetStatus(PUBLISHED)\n");
//break;
//case SYNC_STATUS_MODIFIED:
//printf("ServerObject::SetStatus(MODIFIED)\n");
//break;
//case SYNC_STATUS_SERVER_REMOVED:
//printf("ServerObject::SetStatus(SERVER_REMOVED)\n");
//if (modifiers() & B_SHIFT_KEY)
//debugger("SERVER_REMOVED");
//break;
//case SYNC_STATUS_LOCAL_REMOVED:
//printf("ServerObject::SetStatus(LOCAL_REMOVED)\n");
//if (modifiers() & B_SHIFT_KEY)
//debugger("LOCAL_REMOVED");
//break;
//}
		Notify();
	}
}

// Status
int32
ServerObject::Status() const
{
	if (!fCachedStatusProperty) {
		fCachedStatusProperty = dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_SYNC_STATUS));
	}
	if (!fCachedStatusProperty)
		return SYNC_STATUS_LOCAL;
	return fCachedStatusProperty->Value();
}

// HasRemovedStatus
bool
ServerObject::HasRemovedStatus() const
{
	int32 status = Status();
	return status == SYNC_STATUS_SERVER_REMOVED
		|| status == SYNC_STATUS_LOCAL_REMOVED;
}

// ResolveDependencies
status_t
ServerObject::ResolveDependencies(const ServerObjectManager* library)
{
	return B_OK;
}

// SetMetaDataSaved
void
ServerObject::SetMetaDataSaved(bool saved)
{
	fMetaDataSaved = saved;
}

// SetDataSaved
void
ServerObject::SetDataSaved(bool saved)
{
	if (saved == false && Status() == SYNC_STATUS_PUBLISHED) {
		// if the "data saved" flag is being explicitely being
		// set to "false", it means that we can check the
		// sync status here and make sure it is up 2 date
		SetStatus(SYNC_STATUS_MODIFIED);
	}

	fDataSaved = saved;
}

// #pragma mark -

// AttachedToManager
void
ServerObject::AttachedToManager(ServerObjectManager* manager)
{
	fObjectManager = manager;
}

// DetachedFromManager
void
ServerObject::DetachedFromManager()
{
	fObjectManager = NULL;
}



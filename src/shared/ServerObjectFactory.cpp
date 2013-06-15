/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ServerObjectFactory.h"

#include <new>

#include <String.h>

#include "ServerObject.h"
#include "ServerObjectManager.h"

using std::nothrow;

// constructor
ServerObjectFactory::ServerObjectFactory()
{
}

// destructor
ServerObjectFactory::~ServerObjectFactory()
{
}

// Instantiate
ServerObject*
ServerObjectFactory::Instantiate(BString& type,
								 const BString& serverID,
								 ServerObjectManager* library)
{
	ServerObject* object = new (nothrow) ServerObject(type.String());
	if (object && serverID.Length() > 0)
		object->SetID(serverID);
	return object;
}

// Instantiate
ServerObject*
ServerObjectFactory::InstantiateClone(const ServerObject* other,
									  ServerObjectManager* library)
{
	ServerObject* object = new (nothrow) ServerObject(*other);
	if (object)
		object->SetID(ServerObjectManager::NextID());
	return object;
}

// pragma mark -

// StoreObject
status_t
ServerObjectFactory::StoreObject(ServerObject* object,
	ServerObjectManager* library)
{
	return B_OK;
}

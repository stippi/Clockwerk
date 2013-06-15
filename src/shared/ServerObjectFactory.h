/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SERVER_OBJECT_FACTORY_H
#define SERVER_OBJECT_FACTORY_H

#include <SupportDefs.h>

class BString;
class ServerObject;
class ServerObjectManager;

class ServerObjectFactory {
 public:
								ServerObjectFactory();
	virtual						~ServerObjectFactory();

	virtual	ServerObject*		Instantiate(
									BString& type,
									const BString& serverID,
									ServerObjectManager* library);

	virtual	ServerObject*		InstantiateClone(
									const ServerObject* other,
									ServerObjectManager* library);

	virtual	status_t			StoreObject(ServerObject* object,
									ServerObjectManager* library);
};

#endif // SERVER_OBJECT_FACTORY_H

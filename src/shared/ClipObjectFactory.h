/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_OBJECT_FACTORY_H
#define CLIP_OBJECT_FACTORY_H

#include "ServerObjectFactory.h"

class ClipObjectFactory : public ServerObjectFactory {
 public:
								ClipObjectFactory(bool allowLazyLoading);
	virtual						~ClipObjectFactory();

	virtual	ServerObject*		Instantiate(
									BString& type,
									const BString& serverID,
									ServerObjectManager* library);

	virtual	ServerObject*		InstantiateClone(
									const ServerObject* other,
									ServerObjectManager* library);

	virtual	status_t			StoreObject(ServerObject* object,
									ServerObjectManager* library);

 private:
			bool				fAllowLazyLoading;
};

#endif // CLIP_OBJECT_FACTORY_H

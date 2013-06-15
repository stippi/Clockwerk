/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ATTRIBUTE_SERVER_OBJECT_MANAGER_H
#define ATTRIBUTE_SERVER_OBJECT_MANAGER_H

#include <String.h>

#include "ServerObjectManager.h"

class BDirectory;
class BNode;
class BPath;
class ServerObjectFactory;

class AttributeServerObjectManager : public ServerObjectManager {
 public:
								AttributeServerObjectManager();
	virtual						~AttributeServerObjectManager();

			status_t			Init(const char* directory,
									ServerObjectFactory* factory,
									bool resolveDependencies = true,
									ProgressReporter* reporter = NULL);

	virtual	void				StateChanged();
	virtual void				SetIgnoreStateChanges(bool ignore);
	virtual	bool				IsStateSaved() const;

	virtual	status_t			GetRef(const BString& id, entry_ref& ref);
	virtual status_t			GetRef(const ServerObject* object, entry_ref& ref);

 private:
 			status_t			_GetPath(const BString& id, BPath& path);
			status_t			_CreateObjectFromNode(BNode& node,
									const BString& serverID,
									ServerObjectFactory* factory);
			status_t			_CreateNodeFromObject(
									BDirectory& directory,
									const ServerObject* object) const;

			status_t			_RestorePropertiesFromNode(BNode& node,
									ServerObject* object) const;
			status_t			_StorePropertiesInNode(BNode& node,
									const ServerObject* object) const;

			bool				fIngoreStateChanges;
};

#endif // ATTRIBUTE_SERVER_OBJECT_MANAGER_H

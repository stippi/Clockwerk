/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REFERENCED_OBJECT_FINDER_H
#define REFERENCED_OBJECT_FINDER_H

#include "HashSet.h"
#include "HashString.h"

#include <String.h>

class ServerObjectManager;
class ServerObject;

typedef HashSet<HashString> IdSet;

class ReferencedObjectFinder {
 public:
	static	status_t			FindReferencedObjects(
									ServerObjectManager* library,
									const ServerObject* object,
									IdSet& foundObjects);

 private:
	static	status_t			_GetPlaylistOrScheduleClips(
									ServerObjectManager* library,
									const ServerObject* object,
									IdSet& foundObjects,
									bool schedule,
									bool ignoreReferenced = false);
	static	status_t			_GetRevisionObjects(
									ServerObjectManager* library,
									BString serverID, IdSet& foundObjects);
};

#endif // REFERENCED_OBJECT_FINDER_H

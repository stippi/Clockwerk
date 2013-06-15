/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REMOVE_OBJECTS_COMMAND_H
#define REMOVE_OBJECTS_COMMAND_H

#include "Command.h"

class Selection;
class ServerObjectManager;
class ServerObject;

class RemoveObjectsCommand : public Command {
 public:
								RemoveObjectsCommand(
									ServerObjectManager* library,
									ServerObject** const objects,
									int32 count, Selection* selection,
									bool remove = true);
	virtual						~RemoveObjectsCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			status_t			_RemoveObjects();
			status_t			_UnRemoveObjects();

			void				_RemoveObjectFile(const BString& id,
									bool forReal = false);

			ServerObjectManager* fLibrary;
			ServerObject**		fObjects;
			int32*				fIndices;
			int32				fCount;

			bool				fObjectsRemoved;
			bool				fRemove;

			Selection*			fSelection;
};

#endif // REMOVE_OBJECTS_COMMAND_H

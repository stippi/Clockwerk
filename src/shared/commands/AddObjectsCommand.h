/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ADD_OBJECTS_COMMAND_H
#define ADD_OBJECTS_COMMAND_H

#include "Command.h"

class Selection;
class ServerObjectManager;
class ServerObject;

class AddObjectsCommand : public Command {
 public:
								AddObjectsCommand(
									ServerObjectManager* library,
									ServerObject** const objects,
									int32 count,
									Selection* selection);
	virtual						~AddObjectsCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			void				_RemoveObjectFile(const BString& id);

			ServerObjectManager* fLibrary;
			ServerObject**		fObjects;
			int32				fCount;

			bool				fObjectsAdded;
			Selection*			fSelection;
};

#endif // ADD_OBJECTS_COMMAND_H

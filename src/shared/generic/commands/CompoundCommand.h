/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef COMPOUND_ACTION_H
#define COMPOUND_ACTION_H

#include "Command.h"

class CompoundCommand : public Command {
 public:
								CompoundCommand(Command** commands,
												int32 count,
												const char* name,
												int32 nameIndex);
	virtual						~CompoundCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			Command**			fCommands;
			int32				fCount;

			BString				fName;
			int32				fNameIndex;
};

#endif // COMPOUND_ACTION_H

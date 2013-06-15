/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef COMMAND_STACK_H
#define COMMAND_STACK_H

#include <stack>

#include <Locker.h>

#include "Observable.h"

class BString;
class Command;

class CommandStack : public BLocker,
					 public Observable {
 public:
								CommandStack();
	virtual						~CommandStack();

			status_t			Perform(Command* command);

			status_t			Undo();
			status_t			Redo();

			bool				GetUndoName(BString& name);
			bool				GetRedoName(BString& name);

			void				MakeEmpty();
			void				Save();
			bool				IsSaved();

 private:
			status_t			_AddCommand(Command* command);

	typedef std::stack<Command*> command_stack;

			command_stack			fUndoHistory;
			command_stack			fRedoHistory;
			Command*				fSavedCommand;
};

#endif // COMMAND_STACK_H

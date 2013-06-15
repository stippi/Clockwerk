/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CHANGE_SCHEDULE_ITEM_COMMAND_H
#define CHANGE_SCHEDULE_ITEM_COMMAND_H

#include "Command.h"

class Schedule;
class ScheduleItem;
class Selection;

class ChangeScheduleItemCommand : public Command {
 public:
								ChangeScheduleItemCommand(
									Schedule* schedule,
									Selection* selection,
									ScheduleItem* item,
									uint64 startFrame,
									uint64 duration,
									uint16 repeats,
									bool flexibleStartFrame,
									bool flexibleDuration);
	virtual						~ChangeScheduleItemCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				CombineWithNext(const Command* next);

 private:
			Schedule*			fSchedule;
			Selection*			fSelection;
			ScheduleItem*		fItem;

			uint64				fStartFrame;
			uint64				fDuration;
			uint16				fExplicitRepeats;

			bool				fFlexibleStartFrame : 1;
			bool				fFlexibleDuration : 1;

			enum {
				COMMAND_STARTFRAME			= 1 << 0,
				COMMAND_DURATION			= 1 << 1,
				COMMAND_REPEATS				= 1 << 2,
				COMMAND_FLEXIBLE_STARTFRAME	= 1 << 3,
				COMMAND_FLEXIBLE_DURATION	= 1 << 4,
			};
			uint32				fType;
};

#endif // CHANGE_SCHEDULE_ITEM_COMMAND_H

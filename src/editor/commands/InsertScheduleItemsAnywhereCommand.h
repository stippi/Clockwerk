/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INSERT_SCHEDULE_ITEMS_ANYWHERE_COMMAND_H
#define INSERT_SCHEDULE_ITEMS_ANYWHERE_COMMAND_H

#include "Command.h"

class Schedule;
class ScheduleItem;
class Selection;

class InsertScheduleItemsAnywhereCommand : public Command {
 public:
								InsertScheduleItemsAnywhereCommand(
									Schedule* schedule, Selection* selection,
									ScheduleItem** const items, int32 count,
									uint64 insertFrame,
									int32 insertIndex = -1);
	virtual						~InsertScheduleItemsAnywhereCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Schedule*			fSchedule;
			Selection*			fSelection;
			ScheduleItem**		fItems;

			ScheduleItem*		fAffectedItem;
			uint64				fOriginalAffectedDuration;

			int32				fCount;
			int32				fInsertIndex;

			uint64				fInsertFrame;

			bool				fItemsInserted;
};

#endif // INSERT_SCHEDULE_ITEMS_ANYWHERE_COMMAND_H

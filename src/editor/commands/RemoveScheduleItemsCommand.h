/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REMOVE_SCHEDULE_ITEMS_COMMAND_H
#define REMOVE_SCHEDULE_ITEMS_COMMAND_H

#include "Command.h"

class Schedule;
class ScheduleItem;
class Selection;

// TODO: this is an exact code duplication of DeleteCommand

class RemoveScheduleItemsCommand : public Command {
 public:
								RemoveScheduleItemsCommand(Schedule* schedule,
											  ScheduleItem** items,
											  int32 itemCount,
											  Selection* selection);
								RemoveScheduleItemsCommand(Schedule* schedule,
											  const ScheduleItem** items,
											  int32 itemCount);
	virtual						~RemoveScheduleItemsCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Schedule*			fSchedule;
			ScheduleItem**		fItems;
			int32*				fIndices;
			int32				fItemCount;
			bool				fItemsRemoved;
			Selection*			fSelection;
};

#endif // REMOVE_SCHEDULE_ITEMS_COMMAND_H

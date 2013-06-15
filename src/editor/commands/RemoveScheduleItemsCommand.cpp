/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RemoveScheduleItemsCommand.h"

#include <new>
#include <string.h>

#include "Schedule.h"
#include "ScheduleItem.h"
#include "Selection.h"

using std::nothrow;

// constructor
RemoveScheduleItemsCommand::RemoveScheduleItemsCommand(Schedule* schedule,
	ScheduleItem** items, int32 itemCount, Selection* selection)
	: Command()
	, fSchedule(schedule)
	, fItems(items)
	, fIndices(itemCount > 0 ? new (nothrow) int32[itemCount] : NULL)
	, fItemCount(itemCount)
	, fItemsRemoved(false)
	, fSelection(selection)
{
}

// constructor
RemoveScheduleItemsCommand::RemoveScheduleItemsCommand(Schedule* schedule,
	const ScheduleItem** items, int32 itemCount)
	: Command()
	, fSchedule(schedule)
	, fItems(items && itemCount > 0 ? new (nothrow) ScheduleItem*[itemCount]
									: NULL)
	, fIndices(itemCount > 0 ? new (nothrow) int32[itemCount] : NULL)
	, fItemCount(itemCount)
	, fItemsRemoved(false)
	, fSelection(NULL)
{
	if (fItems)
		memcpy(fItems, items, itemCount * sizeof(ScheduleItem*));
}

// destructor
RemoveScheduleItemsCommand::~RemoveScheduleItemsCommand()
{
	if (fItemsRemoved) {
		// we own the items
		for (int32 i = 0; i < fItemCount; i++)
			delete fItems[i];
	}
	delete[] fItems;
	delete[] fIndices;
}

// InitCheck
status_t
RemoveScheduleItemsCommand::InitCheck()
{
	if (fSchedule && fItems && fIndices)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
RemoveScheduleItemsCommand::Perform()
{
	ScheduleNotificationBlock _(fSchedule);

	for (int32 i = 0; i < fItemCount; i++) {
		if (fSelection)
			fSelection->Deselect(fItems[i]);
		fIndices[i] = fSchedule->IndexOf(fItems[i]);
		fSchedule->RemoveItem(fIndices[i]);
	}

	fSchedule->SanitizeStartFrames();

	fItemsRemoved = true;

	return B_OK;
}

// Undo
status_t
RemoveScheduleItemsCommand::Undo()
{
	status_t ret = B_OK;

	ScheduleNotificationBlock _(fSchedule);

	for (int32 i = fItemCount - 1; i >= 0; i--) {
		if (!fSchedule->AddItem(fItems[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
		if (fSelection)
			fSelection->Select(fItems[i], i != 0);
	}

	fSchedule->SanitizeStartFrames();

	fItemsRemoved = false;

	return ret;
}

// GetName
void
RemoveScheduleItemsCommand::GetName(BString& name)
{
	if (fItemCount > 1)
		name << "Delete Schedule Items";
	else
		name << "Delete Schedule Item";
}


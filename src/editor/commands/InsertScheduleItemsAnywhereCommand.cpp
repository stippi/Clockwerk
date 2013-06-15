/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InsertScheduleItemsAnywhereCommand.h"

#include <new>
#include <stdio.h>

#include "common.h"

#include "Schedule.h"
#include "ScheduleItem.h"
#include "Selection.h"

using std::nothrow;

// constructor
InsertScheduleItemsAnywhereCommand::InsertScheduleItemsAnywhereCommand(
	Schedule* schedule, Selection* selection,
	ScheduleItem** const items, int32 count, uint64 insertFrame,
	int32 insertIndex)
	: Command()
	, fSchedule(schedule)
	, fSelection(selection)
	, fItems(NULL)
	, fAffectedItem(NULL)
	, fOriginalAffectedDuration(0)
	, fCount(0)
	, fInsertIndex(insertIndex)

	, fInsertFrame(insertFrame)

	, fItemsInserted(false)
{
	if (!fSchedule || !items || count <= 0)
		return;

	ScheduleItem* extraFirstItem = NULL;
	ScheduleItem* extraLastItem = NULL;

	uint64 scheduleDuration = 0;
	ScheduleItem* affectedItem = fSchedule->ItemAt(fInsertIndex - 1);
	if (affectedItem) {
		scheduleDuration = affectedItem->StartFrame() + affectedItem->Duration();
		if (affectedItem->StartFrame() < fInsertFrame
			&& scheduleDuration > fInsertFrame) {
			// we need to split this item
			fAffectedItem = affectedItem;
			fOriginalAffectedDuration = fAffectedItem->Duration();
			extraLastItem = new (nothrow) ScheduleItem(*fAffectedItem, true);
			if (extraLastItem) {
				uint64 duration = (scheduleDuration - fInsertFrame);
				extraLastItem->FilterDuration(&duration);
				extraLastItem->SetDuration(duration);
				count++;
			}
		}
	}
	if (fInsertFrame > scheduleDuration) {
		// we need to prepend an empty item
		extraFirstItem = new (nothrow) ScheduleItem((Playlist*)NULL);
		if (extraFirstItem) {
			uint64 duration = fInsertFrame;
			if (ScheduleItem* last = fSchedule->ItemAt(fInsertIndex - 1))
				duration -= last->StartFrame() + last->Duration();
			extraFirstItem->SetDuration(duration);
			count++;
		}
	}

	fCount = count;
	fItems = new (nothrow) ScheduleItem*[fCount];
	if (fItems) {
		// assign fItems, copy items
		ScheduleItem** copyTarget = fItems;
		if (extraFirstItem) {
			fItems[0] = extraFirstItem;
			copyTarget++;
			count--;
		}
		if (extraLastItem) {
			fItems[fCount - 1] = extraLastItem;
			count--;
		}
		memcpy(copyTarget, items, count * sizeof(ScheduleItem*));
	}
}

// destructor
InsertScheduleItemsAnywhereCommand::~InsertScheduleItemsAnywhereCommand()
{
	if (!fItemsInserted) {
		// the ScheduleItems belong to us
		for (int32 i = 0; i < fCount; i++)
			delete fItems[i];
	}
	delete[] fItems;
}

// InitCheck
status_t
InsertScheduleItemsAnywhereCommand::InitCheck()
{
	if (fSchedule && fItems && fCount > 0)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
InsertScheduleItemsAnywhereCommand::Perform()
{
	ScheduleNotificationBlock _(fSchedule);

	int32 insertIndex = fInsertIndex;
	if (insertIndex < 0)
		insertIndex = fSchedule->CountItems();

	// insert items
	uint64 insertFrame = fInsertFrame;
	for (int32 i = 0; i < fCount; i++) {
		// make sure the item is at the right place
		fItems[i]->FilterStartFrame(&insertFrame);
		fItems[i]->SetStartFrame(insertFrame);
		// add it
		if (!fSchedule->AddItem(fItems[i], insertIndex++)) {
			// ERROR - roll back, remove the items
			// we already added
			print_error("InsertScheduleItemsAnywhereCommand::Perform() - "
							"no memory to add items to schedule!\n");
			for (int32 j = i - 1; j >= 0; j--)
				fSchedule->RemoveItem(fItems[j]);
			return B_NO_MEMORY;
		}
		insertFrame += fItems[i]->Duration();

		if (fSelection)
			fSelection->Select(fItems[i], i > 0);
	}
	// set duration of split item
	if (fAffectedItem && fItems[fCount - 1]) {
		uint64 duration
			= fAffectedItem->Duration() - fItems[fCount - 1]->Duration();
		fAffectedItem->FilterDuration(&duration);
		fAffectedItem->SetDuration(duration);
	}

	fSchedule->SanitizeStartFrames();

	fItemsInserted = true;

	return B_OK;
}

// Undo
status_t
InsertScheduleItemsAnywhereCommand::Undo()
{
	ScheduleNotificationBlock _(fSchedule);

	// remove items
	for (int32 i = fCount - 1; i >= 0; i--) {
		if (fSelection)
			fSelection->Deselect(fItems[i]);

		fSchedule->RemoveItem(fItems[i]);
	}
	// set duration of split item
	if (fAffectedItem) {
		fAffectedItem->SetDuration(fOriginalAffectedDuration);
	}

	fSchedule->SanitizeStartFrames();

	fItemsInserted = false;

	return B_OK;
}

// GetName
void
InsertScheduleItemsAnywhereCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Insert Schedule Items";
	else
		name << "Insert Schedule Item";
}

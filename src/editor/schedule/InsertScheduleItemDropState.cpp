/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InsertScheduleItemDropState.h"

#include <new>
#include <stdio.h>

#include "InsertScheduleItemsAnywhereCommand.h"
#include "InsertScheduleItemsCommand.h"
#include "Playlist.h"
#include "ServerObjectManager.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "ScheduleMessages.h"
#include "ScheduleView.h"

using std::nothrow;

// constructor
InsertScheduleItemDropState::InsertScheduleItemDropState(ScheduleView* view)
	: DropAnticipationState(view)
	, fView(view)

	, fDropFrame(0)
	, fDraggedPlaylistDuration(0)
{
}

// destructor
InsertScheduleItemDropState::~InsertScheduleItemDropState()
{
}

// #pragma mark -

// Draw
void
InsertScheduleItemDropState::Draw(BView* into, BRect updateRect)
{
	if (!fDropAnticipationRect.IsValid())
		return;

	BRect r = fDropAnticipationRect;
	r.right--;
	if (r.IsValid()) {
		into->SetHighColor(0, 0, 0, 30);
		into->SetDrawingMode(B_OP_ALPHA);
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		into->FillRect(fDropAnticipationRect);
	}

	into->SetHighColor(255, 0, 0, 255);
	into->SetDrawingMode(B_OP_COPY);
	into->StrokeLine(fDropAnticipationRect.LeftTop(),
					 fDropAnticipationRect.RightTop());
}

// #pragma mark -

// WouldAcceptDragMessage
bool
InsertScheduleItemDropState::WouldAcceptDragMessage(const BMessage* dragMessage)
{
	if (dragMessage->what != MSG_DRAG_PLAYLIST)
		return false;

	// inspect the message to retrieve the clip duration
	ServerObjectManager* library;
	if (dragMessage->FindPointer("library", (void**)&library) != B_OK)
		return false;

	if (!library || !library->ReadLock())
		return false;

	fDraggedPlaylistDuration = 0;

	ServerObject* object;
	for (int32 i = 0; dragMessage->FindPointer("object", i,
											   (void**)&object) == B_OK; i++) {
		if (!library->HasObject(object))
			continue;

		Playlist* playlist = dynamic_cast<Playlist*>(object);
		if (!playlist)
			continue;

		fDraggedPlaylistDuration += playlist->Duration();
	}
	if (fDraggedPlaylistDuration == 0) {
		printf("InsertScheduleItemDropState::WouldAcceptDragMessage() - "
			"unable to obtain duration of dragged object\n");
		fDraggedPlaylistDuration = 60 * 25;
	}

	library->ReadUnlock();

	return true;
}

// HandleDropMessage
Command*
InsertScheduleItemDropState::HandleDropMessage(BMessage* dropMessage)
{
	if (dropMessage->what != MSG_DRAG_PLAYLIST)
		return NULL;

	// inspect the message to retrieve the clips
	ServerObjectManager* library;
	if (dropMessage->FindPointer("library", (void**)&library) != B_OK)
		return NULL;

	if (!library || !library->ReadLock())
		return NULL;

	// temporary list to hold the created items
	BList items;

	ServerObject* object;
	for (int32 i = 0; dropMessage->FindPointer("object", i,
											   (void**)&object) == B_OK; i++) {
		if (!library->HasObject(object)) {
			// the message has arrived asynchronously,
			// so the object pointer might be stale
			continue;
		}

		Playlist* playlist = dynamic_cast<Playlist*>(object);
		if (!playlist)
			continue;

		ScheduleItem* item = new (nothrow) ScheduleItem(playlist);

		if (!items.AddItem(item)) {
			fprintf(stderr, "InsertScheduleItemDropState::HandleDropMessage() "
							"no memory to insert item in list\n");
			delete item;
			break;
		}
	}

	library->ReadUnlock();

	int32 insertIndex = 0;
	Schedule* schedule = fView->Schedule();
	if (schedule)
		insertIndex = schedule->InsertIndexAtFrame(fDropFrame);

	if (fInsertAnywhere) {
		return new (nothrow) InsertScheduleItemsAnywhereCommand(schedule,
			fView->Selection(), (ScheduleItem**)items.Items(),
			items.CountItems(), fDropFrame, insertIndex);
	} else {
		return new (nothrow) InsertScheduleItemsCommand(schedule,
			fView->Selection(), (ScheduleItem**)items.Items(),
			items.CountItems(), fDropFrame, insertIndex);
	}
}

// UpdateDropIndication
void
InsertScheduleItemDropState::UpdateDropIndication(const BMessage* dragMessage,
	BPoint where, uint32 modifiers)
{
	fDropFrame = 0;
	fInsertAnywhere = modifiers & B_COMMAND_KEY;

	Schedule* schedule = fView->Schedule();
	if (!fInsertAnywhere && schedule) {
		float dist = LONG_MAX;
		int32 count = schedule->CountItems();
		fInsertAnywhere = count == 0;
		for (int32 i = 0; i < count; i++) {
			ScheduleItem* item = schedule->ItemAtFast(i);
			BRect r = fView->LayoutScheduleItem(item);
			if (fabs(where.y - r.top) < dist) {
				dist = fabs(where.y - r.top);
				fDropFrame = item->StartFrame();
			}
			if (fabs(where.y - r.bottom) < dist) {
				dist = fabs(where.y - r.top);
				fDropFrame = item->StartFrame() + item->Duration();
			}
		}
	}
	if (fInsertAnywhere) {
		uint32 dropSecond = fView->TimeForHeight(where.y);
		// snap the seconds to half hours
		uint32 halfHours = dropSecond / (60 * 30);
		fDropFrame = (uint64)(halfHours * 60 * 30) * 25;
	}

	uint64 endFrame = fDropFrame + fDraggedPlaylistDuration;
	SetDropAnticipationRect(fView->LayoutScheduleItem(fDropFrame, endFrame));
}




/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PickEndCommand.h"

#include <stdio.h>

#include "support.h"

#include "Playlist.h"
#include "PlaylistItem.h"
#include "SnapFrameList.h"
#include "TimelineView.h"

// constructor
PickEndCommand::PickEndCommand(PlaylistItem* item, int64 dragStartFrame)
	: TrackingCommand(),
	  fItem(item),

	  fLastFrame(dragStartFrame),

	  fEndFrameOffset(0),

	  fPushedBackStart(0),
	  fPushedBackFrames(0)
{
}

// destructor
PickEndCommand::~PickEndCommand()
{
}

// #pragma mark -

// InitCheck
status_t
PickEndCommand::InitCheck()
{
	if (fItem && fEndFrameOffset != 0)
		return B_OK;

	return B_ERROR;
}

// Undo
status_t
PickEndCommand::Undo()
{
	_SetItemEndFrame(fItem->EndFrame() - fEndFrameOffset);
	return B_OK;
}

// Redo
status_t
PickEndCommand::Redo()
{
	_SetItemEndFrame(fItem->EndFrame() + fEndFrameOffset);
	return B_OK;
}

// GetName
void
PickEndCommand::GetName(BString& name)
{
	name << "Change Item End";
}

// #pragma mark -

// Track
void
PickEndCommand::Track(BPoint where, TimelineView* view,
					  SnapFrameList* snapFrames)
{
	int64 dragFrame = view->FrameForPos(where.x);

	int64 newEndFrame = fItem->EndFrame() + (dragFrame - fLastFrame);
	// snapping
	// TODO: make optional
	int64 snappedEndFrame = snapFrames->ClosestFrameFor(newEndFrame,
		fItem->Track(), view->ZoomLevel());
	// since it is the endframe, we need to adjust it
	snappedEndFrame--;

	// enforce item limits
	int64 minEndFrame = fItem->StartFrame();
	int64 maxEndFrame = fItem->StartFrame()
		- fItem->ClipOffset() + fItem->MaxDuration() - 1;
	if (snappedEndFrame < minEndFrame)
		snappedEndFrame = minEndFrame;
	if (snappedEndFrame > maxEndFrame)
		snappedEndFrame = maxEndFrame;

	// remember offsets
	fEndFrameOffset += snappedEndFrame - fItem->EndFrame();

	_SetItemEndFrame(snappedEndFrame);

	SetCurrentFrame(snappedEndFrame);

	// compensate the snap offset
	dragFrame += snappedEndFrame - newEndFrame;
	fLastFrame = dragFrame;
}

// #pragma mark -

// _SetItemEndFrame
void
PickEndCommand::_SetItemEndFrame(int64 endFrame)
{
	Playlist* list = fItem->Parent();
	AutoNotificationSuspender _(list);
	fItem->SuspendNotifications(true);

	// undo any changes to other items on the (old) track
	// (that's the track that our item is currently on)
	if (fPushedBackFrames > 0) {
		list->MoveItems(fPushedBackStart, -fPushedBackFrames,
						fItem->Track(), fItem);
		fPushedBackFrames = 0;
			// at this point, nothing is pushed around anymore
	}

	// do the change to our item
	fItem->SetDuration(endFrame - fItem->StartFrame() + 1);

	// push other items arround to make room
	int64 start = fItem->StartFrame();
	int64 end = start + fItem->Duration();

	list->MakeRoom(start, end, fItem->Track(), fItem,
				   &fPushedBackStart, &fPushedBackFrames);

	// TODO: not nice that this is done here instead of "automatic"
	list->ItemsChanged();
	fItem->SuspendNotifications(false);
}


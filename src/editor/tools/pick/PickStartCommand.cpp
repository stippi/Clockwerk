/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PickStartCommand.h"

#include <stdio.h>

#include "support.h"

#include "Playlist.h"
#include "PlaylistItem.h"
#include "SnapFrameList.h"
#include "TimelineView.h"

// constructor
PickStartCommand::PickStartCommand(PlaylistItem* item, int64 dragStartFrame)
	: TrackingCommand(),
	  fItem(item),

	  fLastFrame(dragStartFrame),

	  fStartFrameOffset(0),

	  fPushedBackStart(0),
	  fPushedBackFrames(0)
{
}

// destructor
PickStartCommand::~PickStartCommand()
{
}

// #pragma mark -

// InitCheck
status_t
PickStartCommand::InitCheck()
{
	if (fItem && fStartFrameOffset != 0)
		return B_OK;

	return B_ERROR;
}

// Undo
status_t
PickStartCommand::Undo()
{
	_SetItemClipOffset(fItem->StartFrame() - fStartFrameOffset);
	return B_OK;
}

// Redo
status_t
PickStartCommand::Redo()
{
	_SetItemClipOffset(fItem->StartFrame() + fStartFrameOffset);
	return B_OK;
}

// GetName
void
PickStartCommand::GetName(BString& name)
{
	name << "Change Item Start";
}

// #pragma mark -

// Track
void
PickStartCommand::Track(BPoint where, TimelineView* view,
						SnapFrameList* snapFrames)
{
	int64 dragFrame = view->FrameForPos(where.x);

	int64 newStartFrame = fItem->StartFrame() + (dragFrame - fLastFrame);
	// snapping
	// TODO: make optional
	int64 snappedStartFrame = snapFrames->ClosestFrameFor(newStartFrame,
		fItem->Track(), view->ZoomLevel());
	// enforce item limits
	int64 minStartFrame = fItem->StartFrame() - fItem->ClipOffset();
	int64 maxStartFrame = fItem->EndFrame();
	if (snappedStartFrame < minStartFrame)
		snappedStartFrame = minStartFrame;
	if (snappedStartFrame > maxStartFrame)
		snappedStartFrame = maxStartFrame;

	// remember offsets
	fStartFrameOffset += snappedStartFrame - fItem->StartFrame();

	_SetItemClipOffset(snappedStartFrame);

	SetCurrentFrame(snappedStartFrame);

	// compensate the snap offset
	dragFrame += snappedStartFrame - newStartFrame;
	fLastFrame = dragFrame;
}

// #pragma mark -

// _SetItemPosition
void
PickStartCommand::_SetItemClipOffset(int64 startFrame)
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
	fItem->SetClipOffset(fItem->ClipOffset() + startFrame - fItem->StartFrame());

	// push other items arround to make room
	int64 start = fItem->StartFrame();
	int64 end = start + fItem->Duration();

	list->MakeRoom(start, end, fItem->Track(), fItem,
				   &fPushedBackStart, &fPushedBackFrames);

	// TODO: not nice that this is done here instead of "automatic"
	list->ItemsChanged();
	fItem->SuspendNotifications(false);
}


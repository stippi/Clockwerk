/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PickMoveCommand.h"

#include <stdio.h>

#include "support.h"

#include "Playlist.h"
#include "PlaylistItem.h"
#include "SnapFrameList.h"
#include "TimelineView.h"

// constructor
PickMoveCommand::PickMoveCommand(PlaylistItem* item, int64 dragStartFrame)
	: TrackingCommand(),
	  fItem(item),

	  fLastFrame(dragStartFrame),

	  fStartFrameOffset(0),
	  fTrackOffset(0),

	  fPushedBackStart(0),
	  fPushedBackFrames(0)
{
}

// destructor
PickMoveCommand::~PickMoveCommand()
{
}

// #pragma mark -

// InitCheck
status_t
PickMoveCommand::InitCheck()
{
	if (fItem && (fStartFrameOffset != 0 || fTrackOffset != 0))
		return B_OK;

	return B_ERROR;
}

// Undo
status_t
PickMoveCommand::Undo()
{
	_SetItemPosition(fItem->StartFrame() - fStartFrameOffset,
					 fItem->Track() - fTrackOffset);
	return B_OK;
}

// Redo
status_t
PickMoveCommand::Redo()
{
	_SetItemPosition(fItem->StartFrame() + fStartFrameOffset,
					 fItem->Track() + fTrackOffset);

	return B_OK;
}

// GetName
void
PickMoveCommand::GetName(BString& name)
{
	name << "Move Item";
}

// #pragma mark -

// Track
void
PickMoveCommand::Track(BPoint where, TimelineView* view,
					   SnapFrameList* snapFrames)
{
	int64 dragFrame = view->FrameForPos(where.x);
	uint32 dragTrack = view->TrackForPos(where.y);

	int64 newStartFrame = fItem->StartFrame() + (dragFrame - fLastFrame);
	// snapping
	// TODO: make optional
	int64 snappedStartFrame = snapFrames->ClosestFrameFor(newStartFrame,
		dragTrack, view->ZoomLevel());

	// remember offsets
	fStartFrameOffset += snappedStartFrame - fItem->StartFrame();
	fTrackOffset += dragTrack - fItem->Track();

	_SetItemPosition(snappedStartFrame, dragTrack);

	// compensate the snap offset
	dragFrame += snappedStartFrame - newStartFrame;
	fLastFrame = dragFrame;
}

// #pragma mark -

// _SetItemPosition
void
PickMoveCommand::_SetItemPosition(int64 startFrame, uint32 track)
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
	fItem->SetTrack(track);
	fItem->SetStartFrame(startFrame);

	// push other items arround to make room
	int64 start = fItem->StartFrame();
	int64 end = start + fItem->Duration();

	list->MakeRoom(start, end, track, fItem,
				   &fPushedBackStart, &fPushedBackFrames);

	// TODO: not nice that this is done here instead of "automatic"
	list->ItemsChanged();
	fItem->SuspendNotifications(false);
}


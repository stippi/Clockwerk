/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PickManipulator.h"

#include <new>

#include <stdio.h>

#include "CurrentFrame.h"
#include "PickEndCommand.h"
#include "PickMoveCommand.h"
#include "PickStartCommand.h"
#include "PlaylistItem.h"
#include "TimelineView.h"

using std::nothrow;

enum {
	TRACKING_NONE = 0,

	TRACKING_MOVE,

	TRACKING_START,
	TRACKING_END,
};

// constructor
PickManipulator::PickManipulator(PlaylistItem* item)
	: SplitManipulator(item),

	  fCommand(NULL),
	  fTrackMode(TRACKING_NONE)
{
}

// destructor
PickManipulator::~PickManipulator()
{
	delete fCommand;
		// just in case we have not performed for some reason
}

// #pragma mark -

// ToolDraw
void
PickManipulator::ToolDraw(BView* into, BRect itemFrame)
{
	if (itemFrame.IntegerWidth() < 4)
		return;

	itemFrame.InsetBy(1, 1);
	int32 dotCount = itemFrame.IntegerHeight() / 3;

	BPoint p(itemFrame.LeftTop());
	for (int32 i = 0; i < dotCount; i++) {
		into->SetHighColor(0, 0, 0, 75);
		into->StrokeLine(p, p);
		p = p + BPoint(1, 1);
		into->SetHighColor(255, 255, 255, 200);
		into->StrokeLine(p, p);
		p = p + BPoint(-1, 2);
	}

	itemFrame.right--;
	if (itemFrame.IntegerWidth() < 6)
		return;

	p = itemFrame.RightTop();
	for (int32 i = 0; i < dotCount; i++) {
		into->SetHighColor(0, 0, 0, 75);
		into->StrokeLine(p, p);
		p = p + BPoint(1, 1);
		into->SetHighColor(255, 255, 255, 200);
		into->StrokeLine(p, p);
		p = p + BPoint(-1, 2);
	}
}

// ToolMouseDown
bool
PickManipulator::ToolMouseDown(BPoint where)
{
	_SetTracking(fTrackMode, fView->FrameForPos(where.x));
	return true;
}

// ToolMouseMoved
void
PickManipulator::ToolMouseMoved(BPoint where)
{
	if (fCommand)
		fCommand->Track(where, fView, &fSnapFrames);
}

// ToolMouseUp
Command*
PickManipulator::ToolMouseUp()
{
	_SetTracking(TRACKING_NONE, 0);
	if (fCommand)
		fCommand->ResetCurrentFrame();
	Command* command = fCommand;
	fCommand = NULL;
		// not ours anymore
	return command;
}

// ToolMouseOver
bool
PickManipulator::ToolMouseOver(BPoint where)
{
	// NOTE: "where" is guaranteed to be within fItemFrame.
	_SetTrackMode(_TrackModeFor(where));
	return true;
}

// ToolIsActive
bool
PickManipulator::ToolIsActive()
{
	return fCommand != NULL;
}

// #pragma mark -

// _TrackModeFor
uint32
PickManipulator::_TrackModeFor(BPoint where) const
{
	uint32 trackMode = TRACKING_MOVE;

	if (fItemFrame.IntegerWidth() < 5) {
		// frame is very small, figure out the optimal tracking mode
		// user probably wants to enlarge the item...
		if (fItem->Duration() < fItem->MaxDuration())
			trackMode = TRACKING_END;
		else if (fItem->ClipOffset() > 0)
			trackMode = TRACKING_START;
	} else if (where.x < fItemFrame.left + 5)
		trackMode = TRACKING_START;
	else if (where.x > fItemFrame.right - 5)
		trackMode = TRACKING_END;

	return trackMode;
}

//_SetTrackMode
void
PickManipulator::_SetTrackMode(uint32 mode)
{
	if (fTrackMode == mode)
		return;

	fTrackMode = mode;
	// TODO: change view cursor
}

// _SetTracking
void
PickManipulator::_SetTracking(uint32 mode, int64 startDragFrame)
{
	// if anything is tracked, collect frames
	// to snap to (magnetic guides)
	if (mode > TRACKING_NONE) {
		if (fCommand) {
			fprintf(stderr, "PickManipulator::_SetTracking() - "
					"switching commands in the middle of tracking?!");
			delete fCommand;
			fCommand = NULL;
		}
		fSnapFrames.CollectSnapFrames(fView->Playlist(),
									  fItem->Duration());
		fSnapFrames.AddSnapFrame(0, fItem->Duration());
		if (fView->IsPaused())
			fSnapFrames.AddSnapFrame(fView->CurrentFrame(), fItem->Duration());

		switch (mode) {
			case TRACKING_MOVE:
				fCommand = new PickMoveCommand(fItem, startDragFrame);
				break;
			case TRACKING_START:
				fCommand = new PickStartCommand(fItem, startDragFrame);
				fCommand->SetCurrentFrame(_View()->CurrentFrameObject());
				break;
			case TRACKING_END:
				fCommand = new PickEndCommand(fItem, startDragFrame);
				fCommand->SetCurrentFrame(_View()->CurrentFrameObject());
				break;
		}

		fView->Invalidate(fItemFrame);
	}
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistItemManipulator.h"

#include <stdio.h>

#include "ClipPlaylistItem.h"
#include "MainWindow.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "TimelineMessages.h"
#include "TimelineView.h"

// constructor
PlaylistItemManipulator::PlaylistItemManipulator(PlaylistItem* item)
	: Manipulator(item),
	  fItem(item),
	  fItemFrame(0.0, 0.0, -1.0, -1.0),

	  fCachedStartFrame(0),
	  fCachedEndFrame(-1),
	  fCachedTrack(-1),
	  fCachedSelected(false),
	  fCachedMuted(false),
	  fCachedName(""),

	  fView(NULL)
{
}

// destructor
PlaylistItemManipulator::~PlaylistItemManipulator()
{
}

// #pragma mark -

// ObjectChanged
void
PlaylistItemManipulator::ObjectChanged(const Observable* object)
{
	if (object == fItem && fView) {

		if ((fCachedStartFrame != fItem->StartFrame()
			 || fCachedEndFrame != fItem->EndFrame()
			 || fCachedSelected != fItem->IsSelected()
			 || fCachedMuted != (fItem->IsVideoMuted() || fItem->IsAudioMuted())
			 || fCachedTrack != (int32)fItem->Track()
			 || fCachedName != fItem->Name())
			&& fView->LockLooper()) {

			fCachedStartFrame = fItem->StartFrame();
			fCachedEndFrame = fItem->EndFrame();
			fCachedSelected = fItem->IsSelected();
			fCachedMuted = (fItem->IsVideoMuted() || fItem->IsAudioMuted());
			fCachedTrack = fItem->Track();
			fCachedName = fItem->Name();

			PlaylistItemManipulator::RebuildCachedData();
				// skip derived classes implementation
	
			fView->UnlockLooper();
		}
	}
}

// #pragma mark -


// DoubleClicked
bool
PlaylistItemManipulator::DoubleClicked(BPoint where)
{
	ClipPlaylistItem* item = dynamic_cast<ClipPlaylistItem*>(fItem);
	if (!item || !fView || !fView->Window())
		return false;

	BMessage message(MSG_SELECT_AND_SHOW_CLIP);
	message.AddPointer("clip", item->Clip());
	fView->Window()->PostMessage(&message);

	return true;
}

// Bounds
BRect
PlaylistItemManipulator::Bounds()
{
	return fItemFrame;
}

// AttachedToView
void
PlaylistItemManipulator::AttachedToView(StateView* view)
{
	fView = dynamic_cast<TimelineView*>(view);
	fItemFrame = _ComputeFrameFor(fItem);
}

// DetachedFromView
void
PlaylistItemManipulator::DetachedFromView(StateView* view)
{
	fView = NULL;
}

// RebuildCachedData
void
PlaylistItemManipulator::RebuildCachedData()
{
	BRect oldItemFrame = fItemFrame;

	fItemFrame = _ComputeFrameFor(fItem);
	fView->Invalidate(oldItemFrame | fItemFrame);
}

// #pragma mark -

// _ComputeFrameFor
BRect
PlaylistItemManipulator::_ComputeFrameFor(PlaylistItem* item) const
{
	BRect frame;
		// an invalid frame

	if (!fView || !item)
		return frame;

	uint32 trackHeight = fView->TrackHeight();

	frame.top = trackHeight * item->Track();
	frame.bottom = frame.top + trackHeight - 1;

	frame.left = fView->PosForFrame(item->StartFrame());
//	frame.right = fView->PosForFrame(item->EndFrame() + 1) - 1;
	frame.right = fView->PosForFrame(item->EndFrame());

	frame.right++;
	frame.bottom++;

	return frame;
}


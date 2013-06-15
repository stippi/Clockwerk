/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TrackInfoView.h"

#include "CommandStack.h"
#include "Playlist.h"
#include "TimelineView.h"

// constructor
TrackInfoView::TrackInfoView(BRect frame)
	: InfoView(frame, "track info view")
	, fPlaylist(NULL)
	, fCommandStack(NULL)
{
}

// destructor
TrackInfoView::~TrackInfoView()
{
	SetPlaylist(NULL);
}

// Draw
void
TrackInfoView::Draw(BRect updateRect)
{
	InfoView::Draw(updateRect);
}

// MouseDown
void
TrackInfoView::MouseDown(BPoint where)
{
}

// MouseUp
void
TrackInfoView::MouseUp(BPoint where)
{
}

// MouseMoved
void
TrackInfoView::MouseMoved(BPoint where, uint32 transit,
					  const BMessage* dragMessage)
{
}

// #pragma mark -

// ObjectChanged
void
TrackInfoView::ObjectChanged(const Observable* object)
{
	if (!LockLooper())
		return;

	if (object == fPlaylist) {
	}

	UnlockLooper();
}

// #pragma mark -

// SetPlaylist
void
TrackInfoView::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	if (fPlaylist)
		fPlaylist->RemoveObserver(this);

	fPlaylist = playlist;

	if (fPlaylist)
		fPlaylist->AddObserver(this);

	Invalidate();
}

// SetCommandStack
void
TrackInfoView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetTimelineView
void
TrackInfoView::SetTimelineView(TimelineView* view)
{
	fTimelineView = view;
}





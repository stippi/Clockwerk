/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TrackHeaderView.h"

#include <Font.h>

#include "CommandStack.h"
#include "Playlist.h"
#include "TimelineView.h"

// constructor
TrackHeaderView::TrackHeaderView(BRect frame)
	: BView(frame, "track header view",
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
	, fPlaylist(NULL)
	, fCommandStack(NULL)
	, fPlaylistName("<no playlist>")
{
	SetViewColor(B_TRANSPARENT_COLOR);
	SetFont(be_bold_font);
}

// destructor
TrackHeaderView::~TrackHeaderView()
{
	SetPlaylist(NULL);
}

// Draw
void
TrackHeaderView::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect r(Bounds());

	SetHighColor(tint_color(base, B_DARKEN_1_TINT));
	StrokeLine(r.LeftTop(), r.RightTop());
	r.top++;

	SetLowColor(tint_color(base, B_LIGHTEN_1_TINT));
	FillRect(r & updateRect, B_SOLID_LOW);

	r.InsetBy(5, 0);

	font_height fh;
	GetFontHeight(&fh);

	BString truncatedName(fPlaylistName);
	be_bold_font->TruncateString(&truncatedName, B_TRUNCATE_MIDDLE, r.Width());
	SetHighColor(0, 0, 0);
	BPoint pos(r.left, r.top + (r.Height() + fh.ascent) / 2.0);
	DrawString(truncatedName.String(), pos);
}

// MouseDown
void
TrackHeaderView::MouseDown(BPoint where)
{
}

// MouseUp
void
TrackHeaderView::MouseUp(BPoint where)
{
}

// MouseMoved
void
TrackHeaderView::MouseMoved(BPoint where, uint32 transit,
					  const BMessage* dragMessage)
{
}

// #pragma mark -

// ObjectChanged
void
TrackHeaderView::ObjectChanged(const Observable* object)
{
	if (!LockLooper())
		return;

	if (object == fPlaylist) {
		if (fPlaylistName != fPlaylist->Name()) {
			fPlaylistName = fPlaylist->Name();
			Invalidate();
		}
	}

	UnlockLooper();
}

// #pragma mark -

// SetPlaylist
void
TrackHeaderView::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	if (fPlaylist)
		fPlaylist->RemoveObserver(this);

	fPlaylist = playlist;

	if (fPlaylist) {
		fPlaylist->AddObserver(this);
		fPlaylistName = fPlaylist->Name();
	} else
		fPlaylistName = "<no playlist>";

	Invalidate();
}

// SetCommandStack
void
TrackHeaderView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetTimelineView
void
TrackHeaderView::SetTimelineView(TimelineView* view)
{
	fTimelineView = view;
}





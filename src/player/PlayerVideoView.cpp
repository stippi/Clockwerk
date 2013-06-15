/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlayerVideoView.h"

#include <Message.h>
#include <Window.h>

#include "PlayerPlaybackNavigator.h"
#include "PlayerWindow.h"
#include "Playlist.h"

// constructor
PlayerVideoView::PlayerVideoView(BRect frame, bool testMode)
	: BView(frame, "video view", B_FOLLOW_ALL, 0)
	, fNavigator(NULL)
	, fPlaylist(NULL)
	, fVideoWidth(frame.IntegerWidth() + 1)
	, fVideoHeight(frame.IntegerHeight() + 1)
	, fTestMode(testMode)
{
	SetViewColor(0, 0, 0, 255);
}

// destructor
PlayerVideoView::~PlayerVideoView()
{
}

// AttachedToWindow
void
PlayerVideoView::AttachedToWindow()
{
	MakeFocus(true);
}

// MouseDown
void
PlayerVideoView::MouseDown(BPoint where)
{
	if (!fPlaylist || !fNavigator)
		return;

	uint32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons",
			(int32*)&buttons) != B_OK) {
		buttons = 0;
	}

	int32 clicks;
	if (Window()->CurrentMessage()->FindInt32("clicks", &clicks) != B_OK)
		clicks = 0;

	BRect bounds = Bounds();
	BRect videoBounds = BRect(0, 0, fVideoWidth - 1, fVideoHeight - 1);
	// convert "where" to video canvas
	if (bounds.IsValid() && videoBounds.IsValid()) {
		where.x /= ((bounds.Width() + 1) / (videoBounds.Width() + 1));
		where.y /= ((bounds.Height() + 1) / (videoBounds.Height() + 1));
	}
	double currentFrame = fNavigator->CurrentFrame();
	if (fPlaylist->MouseDown(where, buttons, videoBounds, currentFrame,
		fNavigator) || !fTestMode)
		return;

	if (clicks == 2)
		Window()->PostMessage(MSG_TOGGLE_FULLSCREEN);
}

// KeyDown
void
PlayerVideoView::KeyDown(const char* bytes, int32 numBytes)
{
	if (!fTestMode) {
		BView::KeyDown(bytes, numBytes);
		return;
	}

	switch (bytes[0]) {
		case 'f':
		case 'F':
			Window()->PostMessage(MSG_TOGGLE_FULLSCREEN);
			break;

		case B_ESCAPE:
			Window()->PostMessage(MSG_EXIT_FULLSCREEN);
			break;

		case 'h':
		case 'H':
			Window()->PostMessage(MSG_TOGGLE_HIDE);
			break;

		default:
			BView::KeyDown(bytes, numBytes);
	}
}

// SetVideoSize
void
PlayerVideoView::SetVideoSize(uint32 width, uint32 height)
{
	fVideoWidth = width;
	fVideoHeight = height;
}

// SetNavigator
void
PlayerVideoView::SetNavigator(PlayerPlaybackNavigator* navigator)
{
	fNavigator = navigator;
}

// SetPlaylist
void
PlayerVideoView::SetPlaylist(Playlist* playlist)
{
	fPlaylist = playlist;
}


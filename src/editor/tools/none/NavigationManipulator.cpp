/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "NavigationManipulator.h"

#include <new>
#include <stdio.h>

#include <Message.h>
#include <View.h>

#include "PlaybackNavigator.h"
#include "Playlist.h"

using std::nothrow;

// constructor
NavigationManipulator::NavigationManipulator(Playlist* playlist)
	: Manipulator(NULL)
	, fPlaylist(playlist)
	, fCurrentFrame(0)
{
}

// destructor
NavigationManipulator::~NavigationManipulator()
{
}

// #pragma mark -

// Draw
void
NavigationManipulator::Draw(BView* into, BRect updateRect)
{
}

// MouseDown
bool
NavigationManipulator::MouseDown(BPoint where)
{
printf("NavigationManipulator::MouseDown(%.1f, %.1f), fPlaylist = %p\n",
	where.x, where.y, fPlaylist);
	if (!fPlaylist)
		return false;
	fView->ConvertToCanvas(&where);

	uint32 buttons = fView->MouseInfo()->buttons;
	BRect canvasBounds(0, 0, fPlaylist->Width() - 1, fPlaylist->Height() - 1);
	double currentFrame = fCurrentFrame;
	PlaybackNavigator* navigator = PlaybackNavigator::Default();

	return fPlaylist->MouseDown(where, buttons, canvasBounds, currentFrame,
		navigator);
}

// MouseMoved
void
NavigationManipulator::MouseMoved(BPoint where)
{
}

// MouseUp
Command*
NavigationManipulator::MouseUp()
{
	return NULL;
}

// MouseOver
bool
NavigationManipulator::MouseOver(BPoint where)
{
	return false;
}

// DoubleClicked
bool
NavigationManipulator::DoubleClicked(BPoint where)
{
	return false;
}

// #pragma mark -

// ModifiersChanged
void
NavigationManipulator::ModifiersChanged(uint32 modifiers)
{
}

// MessageReceived
bool
NavigationManipulator::MessageReceived(BMessage* message, Command** _command)
{
	switch (message->what) {
		default:
			return Manipulator::MessageReceived(message, _command);
	}
}

// HandleKeyDown
bool
NavigationManipulator::HandleKeyDown(const StateView::KeyEvent& event,
								Command** _command)
{
	switch (event.key) {
		case B_HOME:
			break;
		case B_END:
			break;

		case B_PAGE_UP:
		case B_PAGE_DOWN:
			return false;

		case B_UP_ARROW:
			break;
		case B_DOWN_ARROW:
			break;
		case B_LEFT_ARROW:
			break;
		case B_RIGHT_ARROW:
			break;

		default:
			break;
	}

	return true;
}

// HandleKeyUp
bool
NavigationManipulator::HandleKeyUp(const StateView::KeyEvent& event,
							  Command** _command)
{
	return false;
}

// HandlesAllKeyEvents
bool
NavigationManipulator::HandlesAllKeyEvents() const
{
	return false;
}

// UpdateCursor
bool
NavigationManipulator::UpdateCursor()
{
	// TODO...
	return false;
}

// #pragma mark -

// Bounds
BRect
NavigationManipulator::Bounds()
{
	return fView->Bounds();
}

// ObjectChanged
void
NavigationManipulator::ObjectChanged(const Observable*)
{
	// cannot reach here, since no object is being observed
}

// SetCurrentFrame
void
NavigationManipulator::SetCurrentFrame(int64 frame)
{
	fCurrentFrame = frame;
}


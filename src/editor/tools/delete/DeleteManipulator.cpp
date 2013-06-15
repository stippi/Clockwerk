/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DeleteManipulator.h"

#include <new>
#include <stdio.h>

#include "DeleteCommand.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "TimelineView.h"

using std::nothrow;

// constructor
DeleteManipulator::DeleteManipulator(PlaylistItem* item)
	: SplitManipulator(item),
	  fMouseInsideItem(false)
{
}

// destructor
DeleteManipulator::~DeleteManipulator()
{
}

// #pragma mark -

// ToolMouseDown
bool
DeleteManipulator::ToolMouseDown(BPoint where)
{
	_SetMouseInside(true);
	return true;
}

// ToolMouseMoved
void
DeleteManipulator::ToolMouseMoved(BPoint where)
{
	_SetMouseInside(Bounds().Contains(where));
}

// ToolMouseUp
Command*
DeleteManipulator::ToolMouseUp()
{
	Command* command = NULL;
	if (fMouseInsideItem) {
		PlaylistItem** items = new (nothrow) PlaylistItem*[1];
		if (items) {
			items[0] = fItem;
			command = new (nothrow) DeleteCommand(fView->Playlist(),
												  items, 1,
												  fView->Selection());
		}
		// NOTE: we are toast now!
	}
	return command;
}

// ToolMouseOver
bool
DeleteManipulator::ToolMouseOver(BPoint where)
{
	// NOTE: "where" is guaranteed to be within fItemFrame.
	return true;
}

// ToolIsActive
bool
DeleteManipulator::ToolIsActive()
{
	return fMouseInsideItem;
}

// #pragma mark -

// _SetMouseInside
void
DeleteManipulator::_SetMouseInside(bool inside)
{
	if (fMouseInsideItem != inside) {
		fMouseInsideItem = inside;
		fView->Invalidate(fItemFrame);
	}
}



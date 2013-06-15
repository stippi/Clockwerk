/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CutManipulator.h"

#include <new>
#include <stdio.h>

#include <Font.h>
#include <String.h>

#include "support.h"

#include "CutCommand.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "TimelineView.h"

using std::nothrow;

enum {
	TRACKING_NONE = 0,

	TRACKING_CUT_POS,
};

// constructor
CutManipulator::CutManipulator(PlaylistItem* item)
	: SplitManipulator(item),
	  fCommand(NULL)
{
}

// destructor
CutManipulator::~CutManipulator()
{
	delete fCommand;
}

// #pragma mark -

// ToolMouseDown
bool
CutManipulator::ToolMouseDown(BPoint where)
{
	fCommand = new CutCommand(fItem, fView->FrameForPos(where.x));

	return true;
}

// ToolMouseMoved
void
CutManipulator::ToolMouseMoved(BPoint where)
{
	// TODO: ...
	// (in case CutCommand is supposed to become a TrackingCommand)
}

// ToolMouseUp
Command*
CutManipulator::ToolMouseUp()
{
	Command* command = fCommand;
	fCommand = NULL;

	return command;
}

// ToolMouseOver
bool
CutManipulator::ToolMouseOver(BPoint where)
{
	// NOTE: "where" is guaranteed to be within fItemFrame.
	return true;
}

// ToolIsActive
bool
CutManipulator::ToolIsActive()
{
	return fCommand != NULL;
}


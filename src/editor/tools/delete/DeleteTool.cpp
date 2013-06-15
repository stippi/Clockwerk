/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DeleteTool.h"

#include <stdio.h>

#include "ConfigView.h"
#include "IconButton.h"
#include "Icons.h"
#include "DeleteManipulator.h"

// constructor
DeleteTool::DeleteTool()
	: TimelineTool("delete tool")
{
}

// destructor
DeleteTool::~DeleteTool()
{
}

// #pragma mark -

// SaveSettings
status_t
DeleteTool::SaveSettings(BMessage* message)
{
	return Tool::SaveSettings(message);
}

// LoadSettings
status_t
DeleteTool::LoadSettings(BMessage* message)
{
	return Tool::LoadSettings(message);
}

// ShortHelpMessage
const char*
DeleteTool::ShortHelpMessage()
{
	// TODO: localize
	return "Click clips to remove them from the timeline.";
}

// #pragma mark -

// ManipulatorFor
PlaylistItemManipulator*
DeleteTool::ManipulatorFor(PlaylistItem* item)
{
	return new DeleteManipulator(item);
}

// #pragma mark -

// MakeConfigView
::ConfigView*
DeleteTool::MakeConfigView()
{
	return new ::ConfigView(this);
}

// MakeIcon
IconButton*
DeleteTool::MakeIcon()
{
	IconButton* icon = new IconButton("delete icon", 0, NULL, NULL);
	icon->SetIcon(kTrashIcon, kIconWidth, kIconHeight, kIconFormat);
	return icon;
}

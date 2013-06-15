/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PickTool.h"

#include <stdio.h>

#include "ConfigView.h"
#include "IconButton.h"
#include "Icons.h"
#include "PickManipulator.h"

// constructor
PickTool::PickTool()
	: TimelineTool("pick tool")
{
}

// destructor
PickTool::~PickTool()
{
}

// #pragma mark -

// SaveSettings
status_t
PickTool::SaveSettings(BMessage* message)
{
	return Tool::SaveSettings(message);
}

// LoadSettings
status_t
PickTool::LoadSettings(BMessage* message)
{
	return Tool::LoadSettings(message);
}

// ShortHelpMessage
const char*
PickTool::ShortHelpMessage()
{
	// TODO: localize
	return "Drag clips around on the time line and modify their properties.";
}

// #pragma mark -

// ManipulatorFor
PlaylistItemManipulator*
PickTool::ManipulatorFor(PlaylistItem* item)
{
	return new PickManipulator(item);
}

// #pragma mark -

// MakeConfigView
::ConfigView*
PickTool::MakeConfigView()
{
	return new ::ConfigView(this);
}

// MakeIcon
IconButton*
PickTool::MakeIcon()
{
	IconButton* icon = new IconButton("pick icon", 0, NULL, NULL);
	icon->SetIcon(kPickIcon, kIconWidth, kIconHeight, kIconFormat);
	return icon;
}

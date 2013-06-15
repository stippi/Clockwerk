/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CutTool.h"

#include <stdio.h>

#include "ConfigView.h"
#include "CutManipulator.h"
#include "IconButton.h"
#include "Icons.h"

// constructor
CutTool::CutTool()
	: TimelineTool("cut tool")
{
}

// destructor
CutTool::~CutTool()
{
}

// #pragma mark -

// SaveSettings
status_t
CutTool::SaveSettings(BMessage* message)
{
	return Tool::SaveSettings(message);
}

// LoadSettings
status_t
CutTool::LoadSettings(BMessage* message)
{
	return Tool::LoadSettings(message);
}

// ShortHelpMessage
const char*
CutTool::ShortHelpMessage()
{
	// TODO: localize
	return "Cut clips on the timeline.";
}

// #pragma mark -

// ManipulatorFor
PlaylistItemManipulator*
CutTool::ManipulatorFor(PlaylistItem* item)
{
	return new CutManipulator(item);
}

// #pragma mark -

// MakeConfigView
::ConfigView*
CutTool::MakeConfigView()
{
	return new ::ConfigView(this);
}

// MakeIcon
IconButton*
CutTool::MakeIcon()
{
	IconButton* icon = new IconButton("cut icon", 0, NULL, NULL);
	icon->SetIcon(kCutIcon, kIconWidth, kIconHeight, kIconFormat);
	return icon;
}

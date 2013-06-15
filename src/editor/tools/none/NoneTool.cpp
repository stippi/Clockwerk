/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "NoneTool.h"

#include <new>
#include <stdio.h>

#include "ConfigView.h"
#include "IconButton.h"
#include "Icons.h"
#include "MultipleManipulatorState.h"
#include "NavigationManipulator.h"

using std::nothrow;

// constructor
NoneTool::NoneTool()
	: StageTool("none tool")
{
}

// destructor
NoneTool::~NoneTool()
{
}

// #pragma mark -

// SaveSettings
status_t
NoneTool::SaveSettings(BMessage* message)
{
	return Tool::SaveSettings(message);
}

// LoadSettings
status_t
NoneTool::LoadSettings(BMessage* message)
{
	return Tool::LoadSettings(message);
}

// ShortHelpMessage
const char*
NoneTool::ShortHelpMessage()
{
	// TODO: localize
	return "No Tool. Just watch and navigate the Playlist.";
}

// #pragma mark -

// ManipulatorFor
bool
NoneTool::AddManipulators(StateView* view,
	MultipleManipulatorState* viewState,
	PlaylistItem** const items, int32 count,
	Playlist* playlist, VideoViewSelection* stageSelection)
{
	Manipulator* manipulator = new (nothrow) NavigationManipulator(playlist);
	if (!manipulator || !viewState->AddManipulator(manipulator)) {
		delete manipulator;
		return false;
	}

	return true;
}

// #pragma mark -

// MakeConfigView
::ConfigView*
NoneTool::MakeConfigView()
{
	return new ::ConfigView(this);
}

// MakeIcon
IconButton*
NoneTool::MakeIcon()
{
	IconButton* icon = new IconButton("no tool icon", 0, NULL, NULL);
	icon->SetIcon(kWatchIcon, kIconWidth, kIconHeight, kIconFormat);
	return icon;
}

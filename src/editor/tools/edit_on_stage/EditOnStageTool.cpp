/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "EditOnStageTool.h"

#include <new>
#include <stdio.h>

#include "ConfigView.h"
#include "IconButton.h"
#include "Icons.h"
#include "MultipleManipulatorState.h"
#include "ClipPlaylistItem.h"
#include "TableClip.h"
#include "TableManipulator.h"
#include "TextClip.h"
#include "TextManipulator.h"
#include "VideoViewSelection.h"

// constructor
EditOnStageTool::EditOnStageTool()
	: StageTool("edit on stage tool")
{
}

// destructor
EditOnStageTool::~EditOnStageTool()
{
}

// #pragma mark -

// SaveSettings
status_t
EditOnStageTool::SaveSettings(BMessage* message)
{
	return Tool::SaveSettings(message);
}

// LoadSettings
status_t
EditOnStageTool::LoadSettings(BMessage* message)
{
	return Tool::LoadSettings(message);
}

// ShortHelpMessage
const char*
EditOnStageTool::ShortHelpMessage()
{
	// TODO: localize
	return "Edit selected objects on the Stage.";
}

// #pragma mark -

// ManipulatorFor
bool
EditOnStageTool::AddManipulators(StateView* view,
							   MultipleManipulatorState* viewState,
							   PlaylistItem** const items, int32 count,
							   Playlist* playlist,
							   VideoViewSelection* stageSelection)
{
	if (count <= 0)
		return false;

	ClipPlaylistItem* clipItem
		= dynamic_cast<ClipPlaylistItem*>(items[0]);
	TableClip* table
		= dynamic_cast<TableClip*>(clipItem ? clipItem->Clip() : NULL);
	TextClip* text
		= dynamic_cast<TextClip*>(clipItem ? clipItem->Clip() : NULL);

	Manipulator* manipulator = NULL;

	if (table) {
		manipulator
			= new (std::nothrow) TableManipulator(items[0], table,
				stageSelection);
	} else if (text) {
		manipulator
			= new (std::nothrow) TextManipulator(items[0], text,
				stageSelection);
	}

	if (!manipulator || !viewState->AddManipulator(manipulator)) {
		delete manipulator;
		return false;
	}

	stageSelection->SetAssociatedSelectable(items[0]);

	return true;
}

// #pragma mark -

// MakeConfigView
::ConfigView*
EditOnStageTool::MakeConfigView()
{
	return new ::ConfigView(this);
}

// MakeIcon
IconButton*
EditOnStageTool::MakeIcon()
{
	IconButton* icon = new IconButton("edit icon", 0, NULL, NULL);
	icon->SetIcon(kEditIcon, kIconWidth, kIconHeight, kIconFormat);
	return icon;
}

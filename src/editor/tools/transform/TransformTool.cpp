/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "TransformTool.h"

#include <stdio.h>

#include "ConfigView.h"
#include "IconButton.h"
#include "Icons.h"
#include "MultipleManipulatorState.h"
#include "ObjectSelection.h"

// constructor
TransformTool::TransformTool()
	: StageTool("transform tool")
{
}

// destructor
TransformTool::~TransformTool()
{
}

// #pragma mark -

// SaveSettings
status_t
TransformTool::SaveSettings(BMessage* message)
{
	return Tool::SaveSettings(message);
}

// LoadSettings
status_t
TransformTool::LoadSettings(BMessage* message)
{
	return Tool::LoadSettings(message);
}

// ShortHelpMessage
const char*
TransformTool::ShortHelpMessage()
{
	// TODO: localize
	return "Transform Playlist clips on the video canvas.";
}

// #pragma mark -

// ManipulatorFor
bool
TransformTool::AddManipulators(StateView* view,
							   MultipleManipulatorState* viewState,
							   PlaylistItem** const items, int32 count,
							   Playlist* playlist,
							   VideoViewSelection* stageSelection)
{
	ObjectSelection* manipulator = new ObjectSelection(view, items, count);
	if (!manipulator->Box().IsValid()
		|| !viewState->AddManipulator(manipulator)) {
		// we've got untransformable objects or some other error...
		delete manipulator;
		return false;
	}

	return true;
}

// #pragma mark -

// MakeConfigView
::ConfigView*
TransformTool::MakeConfigView()
{
	return new ::ConfigView(this);
}

// MakeIcon
IconButton*
TransformTool::MakeIcon()
{
	IconButton* icon = new IconButton("transform icon", 0, NULL, NULL);
	icon->SetIcon(kTransformIcon, kIconWidth, kIconHeight, kIconFormat);
	return icon;
}

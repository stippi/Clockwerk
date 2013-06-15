/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "EditorPlaybackNavigator.h"

#include "EditorApp.h"
#include "MessageConstants.h"
#include "NavigationInfo.h"

// constructor
EditorPlaybackNavigator::EditorPlaybackNavigator(EditorApp* app)
	: PlaybackNavigator()
	, fApplication(app)
{
}

// destructor
EditorPlaybackNavigator::~EditorPlaybackNavigator()
{
}

// Navigate
void
EditorPlaybackNavigator::Navigate(const NavigationInfo* info)
{
	if (info == NULL || info->TargetID() == NULL)
		return;

	BMessage message(MSG_OPEN);
	message.AddString("soid", info->TargetID());

	fApplication->PostMessage(&message);
}


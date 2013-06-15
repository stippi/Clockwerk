/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SetAudioMutedCommand.h"

#include <stdio.h>

#include "PlaylistItem.h"

// constructor
SetAudioMutedCommand::SetAudioMutedCommand(PlaylistItem* item)
	: Command()
	, fItem(item)
{
}

// destructor
SetAudioMutedCommand::~SetAudioMutedCommand()
{
}

// #pragma mark -

// InitCheck
status_t
SetAudioMutedCommand::InitCheck()
{
	if (fItem && fItem->HasAudio())
		return B_OK;
	return B_ERROR;
}

// Perform
status_t
SetAudioMutedCommand::Perform()
{
	fItem->SetAudioMuted(!fItem->IsAudioMuted());
	return B_OK;
}

// Undo
status_t
SetAudioMutedCommand::Undo()
{
	return Perform();
}

// GetName
void
SetAudioMutedCommand::GetName(BString& name)
{
	name << "Toggle Video Muted";
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SetVideoMutedCommand.h"

#include <stdio.h>

#include "PlaylistItem.h"

// constructor
SetVideoMutedCommand::SetVideoMutedCommand(PlaylistItem* item)
	: Command()
	, fItem(item)
{
}

// destructor
SetVideoMutedCommand::~SetVideoMutedCommand()
{
}

// #pragma mark -

// InitCheck
status_t
SetVideoMutedCommand::InitCheck()
{
	if (fItem && fItem->HasVideo())
		return B_OK;
	return B_ERROR;
}

// Perform
status_t
SetVideoMutedCommand::Perform()
{
	fItem->SetVideoMuted(!fItem->IsVideoMuted());
	return B_OK;
}

// Undo
status_t
SetVideoMutedCommand::Undo()
{
	return Perform();
}

// GetName
void
SetVideoMutedCommand::GetName(BString& name)
{
	name << "Toggle Video Muted";
}


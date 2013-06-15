/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CloseGapCommand.h"

#include "Playlist.h"
#include "PlaylistItem.h"

// constructor
CloseGapCommand::CloseGapCommand(Playlist* playlist,uint32 track, 
		int64 gapStartFrame, int64 gapEndFrame)
	: Command()
	, fPlaylist(playlist)
	, fTrack(track)
	, fGapStartFrame(gapStartFrame)
	, fGapEndFrame(gapEndFrame)
{
}

// destructor
CloseGapCommand::~CloseGapCommand()
{
}

// InitCheck
status_t
CloseGapCommand::InitCheck()
{
	if (fPlaylist && fGapStartFrame < fGapEndFrame)
		return B_OK;
	return B_ERROR;
}

// Perform
status_t
CloseGapCommand::Perform()
{
	PlaylistNotificationBlock _(fPlaylist);

	int64 offset = fGapEndFrame - fGapStartFrame;
	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = fPlaylist->ItemAtFast(i);
		if (item->Track() == fTrack && item->StartFrame() > fGapStartFrame) {
			item->SetStartFrame(item->StartFrame() - offset);
		}
	}
	return B_OK;
}

// Undo
status_t
CloseGapCommand::Undo()
{
	PlaylistNotificationBlock _(fPlaylist);

	int64 offset = fGapEndFrame - fGapStartFrame;
	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = fPlaylist->ItemAtFast(i);
		if (item->Track() == fTrack && item->StartFrame() >= fGapStartFrame) {
			item->SetStartFrame(item->StartFrame() + offset);
		}
	}
	return B_OK;
}

// GetName
void
CloseGapCommand::GetName(BString& name)
{
	name << "Close Gap";
}


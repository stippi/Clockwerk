/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "MoveTrackCommand.h"

#include "Playlist.h"

// constructor
MoveTrackCommand::MoveTrackCommand(Playlist* playlist,
								   uint32 oldIndex, uint32 newIndex)
	: Command(),
	  fPlaylist(playlist),
	  fOldIndex(oldIndex),
	  fNewIndex(newIndex)
{
}

// destructor
MoveTrackCommand::~MoveTrackCommand()
{
}

// InitCheck
status_t
MoveTrackCommand::InitCheck()
{
	return fOldIndex != fNewIndex ? B_OK : B_NO_INIT;
}

// Perform
status_t
MoveTrackCommand::Perform()
{
	fPlaylist->MoveTrack(fOldIndex, fNewIndex);

	return B_OK;
}

// Undo
status_t
MoveTrackCommand::Undo()
{
	fPlaylist->MoveTrack(fNewIndex, fOldIndex);

	return B_OK;
}

// GetName
void
MoveTrackCommand::GetName(BString& name)
{
	name << "Move Track";
}

// UndoesPrevious
bool
MoveTrackCommand::UndoesPrevious(const Command* previous) const
{
	const MoveTrackCommand* other
		= dynamic_cast<const MoveTrackCommand*>(previous);
	if (other && other->fPlaylist == fPlaylist
		&& other->fOldIndex == fNewIndex && other->fNewIndex == fOldIndex) {
		return true;
	}
	return false;
}

// CombineWithNext
bool
MoveTrackCommand::CombineWithNext(const Command* next)
{
	const MoveTrackCommand* other
		= dynamic_cast<const MoveTrackCommand*>(next);
	if (other && other->fTimeStamp - fTimeStamp < 500000
		&& other->fPlaylist == fPlaylist
		&& other->fOldIndex == fNewIndex) {
		fNewIndex = other->fNewIndex;
		fTimeStamp = other->fTimeStamp;
		return true;
	}
	return false;
}


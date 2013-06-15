/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SetSoloTrackCommand.h"

#include "Playlist.h"

// constructor
SetSoloTrackCommand::SetSoloTrackCommand(Playlist* playlist,
										 int32 newSoloTrack)
	: Command(),
	  fPlaylist(playlist),
	  fOldSoloTrack(newSoloTrack),
	  fNewSoloTrack(newSoloTrack)
{
	if (fPlaylist)
		fOldSoloTrack = fPlaylist->SoloTrack();
}

// destructor
SetSoloTrackCommand::~SetSoloTrackCommand()
{
}

// InitCheck
status_t
SetSoloTrackCommand::InitCheck()
{
	return fOldSoloTrack != fNewSoloTrack ? B_OK : B_NO_INIT;
}

// Perform
status_t
SetSoloTrackCommand::Perform()
{
	fPlaylist->SetSoloTrack(fNewSoloTrack);

	return B_OK;
}

// Undo
status_t
SetSoloTrackCommand::Undo()
{
	fPlaylist->SetSoloTrack(fOldSoloTrack);

	return B_OK;
}

// GetName
void
SetSoloTrackCommand::GetName(BString& name)
{
	if (fNewSoloTrack >= 0 && fOldSoloTrack >= 0)
		name << "Change Solo Track";
	else if (fNewSoloTrack >= 0)
		name << "Set Solo Track";
	else
		name << "Disable Solo Track";
}

// UndoesPrevious
bool
SetSoloTrackCommand::UndoesPrevious(const Command* previous) const
{
	const SetSoloTrackCommand* other
		= dynamic_cast<const SetSoloTrackCommand*>(previous);
	if (other && other->fPlaylist == fPlaylist)
		return other->fOldSoloTrack == fNewSoloTrack;
	return false;
}

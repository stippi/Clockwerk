/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InsertOrRemoveTrackCommand.h"

#include "Playlist.h"

// constructor
InsertOrRemoveTrackCommand::InsertOrRemoveTrackCommand(Playlist* playlist,
													   uint32 track,
													   bool insert)
	: Command(),
	  fPlaylist(playlist),
	  fTrack(track),
	  fInsert(insert),
	  fTrackProperties(0UL)
{
	if (fPlaylist && !insert) {
		TrackProperties* original = fPlaylist->PropertiesForTrack(fTrack);
		if (original)
			fTrackProperties = *original;
	}
}

// destructor
InsertOrRemoveTrackCommand::~InsertOrRemoveTrackCommand()
{
}

// InitCheck
status_t
InsertOrRemoveTrackCommand::InitCheck()
{
	return fPlaylist ? B_OK : B_NO_INIT;
}

// Perform
status_t
InsertOrRemoveTrackCommand::Perform()
{
	if (fInsert)
		fPlaylist->InsertTrack(fTrack);
	else
		fPlaylist->RemoveTrack(fTrack);

	return B_OK;
}

// Undo
status_t
InsertOrRemoveTrackCommand::Undo()
{
	if (fInsert)
		fPlaylist->RemoveTrack(fTrack);
	else {
		fPlaylist->InsertTrack(fTrack);
		if (!fTrackProperties.IsDefault())
			fPlaylist->SetTrackProperties(fTrackProperties);
	}

	return B_OK;
}

// GetName
void
InsertOrRemoveTrackCommand::GetName(BString& name)
{
	if (fInsert)
		name << "Insert Track " << fTrack + 1;
	else
		name << "Remove Track " << fTrack + 1;
}

// UndoesPrevious
bool
InsertOrRemoveTrackCommand::UndoesPrevious(const Command* previous) const
{
	const InsertOrRemoveTrackCommand* other
		= dynamic_cast<const InsertOrRemoveTrackCommand*>(previous);
	if (other && other->fPlaylist == fPlaylist)
		return other->fTrack == fTrack && other->fInsert != fInsert;
	return false;
}

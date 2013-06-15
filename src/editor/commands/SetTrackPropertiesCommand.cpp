/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SetTrackPropertiesCommand.h"

#include "Playlist.h"

// constructor
SetTrackPropertiesCommand::SetTrackPropertiesCommand(Playlist* playlist,
									const TrackProperties& oldProperties,
									const TrackProperties& newProperties)
	: Command(),
	  fPlaylist(playlist),
	  fOldProperties(oldProperties),
	  fNewProperties(newProperties)
{
}

// destructor
SetTrackPropertiesCommand::~SetTrackPropertiesCommand()
{
}

// InitCheck
status_t
SetTrackPropertiesCommand::InitCheck()
{
	return fPlaylist && fOldProperties != fNewProperties
		&& fOldProperties.Track() == fNewProperties.Track() ? B_OK : B_NO_INIT;
}

// Perform
status_t
SetTrackPropertiesCommand::Perform()
{
	if (!fNewProperties.IsDefault())
		fPlaylist->SetTrackProperties(fNewProperties);
	else
		fPlaylist->ClearTrackProperties(fNewProperties.Track());

	return B_OK;
}

// Undo
status_t
SetTrackPropertiesCommand::Undo()
{
	if (!fOldProperties.IsDefault())
		fPlaylist->SetTrackProperties(fOldProperties);
	else
		fPlaylist->ClearTrackProperties(fOldProperties.Track());

	return B_OK;
}

// GetName
void
SetTrackPropertiesCommand::GetName(BString& name)
{
	name << "Change Properties of Track " << fOldProperties.Track() + 1;
}

// UndoesPrevious
bool
SetTrackPropertiesCommand::UndoesPrevious(const Command* previous) const
{
	const SetTrackPropertiesCommand* other
		= dynamic_cast<const SetTrackPropertiesCommand*>(previous);
	if (other && other->fPlaylist == fPlaylist)
		return other->fOldProperties == fNewProperties;
	return false;
}



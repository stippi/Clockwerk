/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SET_TRACK_PROPERTIES_COMMAND_H
#define SET_TRACK_PROPERTIES_COMMAND_H

#include "Command.h"
#include "TrackProperties.h"

class Playlist;

class SetTrackPropertiesCommand : public Command {
 public:
								SetTrackPropertiesCommand(Playlist* playlist,
									const TrackProperties& oldProperties,
									const TrackProperties& newProperties);
	virtual						~SetTrackPropertiesCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				UndoesPrevious(const Command* previous) const;

 private:
			Playlist*			fPlaylist;

			TrackProperties		fOldProperties;
			TrackProperties		fNewProperties;
};

#endif // SET_TRACK_PROPERTIES_COMMAND_H

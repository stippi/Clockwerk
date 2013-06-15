/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SET_SOLO_TRACK_COMMAND_H
#define SET_SOLO_TRACK_COMMAND_H

#include "Command.h"

class Playlist;

class SetSoloTrackCommand : public Command {
 public:
								SetSoloTrackCommand(Playlist* playlist,
									int32 newSoloTrack);
	virtual						~SetSoloTrackCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				UndoesPrevious(const Command* previous) const;

 private:
			Playlist*			fPlaylist;

			int32				fOldSoloTrack;
			int32				fNewSoloTrack;
};

#endif // SET_SOLO_TRACK_COMMAND_H

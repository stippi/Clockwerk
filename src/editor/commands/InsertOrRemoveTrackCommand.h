/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INSERT_OR_REMOVE_TRACK_COMMAND_H
#define INSERT_OR_REMOVE_TRACK_COMMAND_H

#include "Command.h"
#include "TrackProperties.h"

class Playlist;

class InsertOrRemoveTrackCommand : public Command {
 public:
								InsertOrRemoveTrackCommand(Playlist* playlist,
									uint32 track, bool insert);
	virtual						~InsertOrRemoveTrackCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				UndoesPrevious(const Command* previous) const;

 private:
			Playlist*			fPlaylist;

			uint32				fTrack;
			bool				fInsert;
			TrackProperties		fTrackProperties;
};

#endif // INSERT_OR_REMOVE_TRACK_COMMAND_H

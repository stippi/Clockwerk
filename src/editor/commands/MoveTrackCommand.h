/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef MOVE_TRACK_COMMAND_H
#define MOVE_TRACK_COMMAND_H

#include "Command.h"

class Playlist;

class MoveTrackCommand : public Command {
 public:
								MoveTrackCommand(Playlist* playlist,
									uint32 oldIndex, uint32 newIndex);
	virtual						~MoveTrackCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

	virtual	bool				UndoesPrevious(const Command* previous) const;
	virtual	bool				CombineWithNext(const Command* next);

 private:
			Playlist*			fPlaylist;

			uint32				fOldIndex;
			uint32				fNewIndex;
};

#endif // SET_SOLO_TRACK_COMMAND_H

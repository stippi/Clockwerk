/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLOSE_GAP_COMMAND_H
#define CLOSE_GAP_COMMAND_H

#include "Command.h"

class Playlist;

class CloseGapCommand : public Command {
 public:
								CloseGapCommand(Playlist* playlist,
									uint32 track, int64 gapStartFrame,
									int64 gapEndFrame);
	virtual						~CloseGapCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;

			uint32				fTrack;
			int64				fGapStartFrame;
			int64				fGapEndFrame;
};

#endif // CLOSE_GAP_COMMAND_H

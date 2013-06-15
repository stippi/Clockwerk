/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INSERT_COMMAND_H
#define INSERT_COMMAND_H

#include "Command.h"

class Playlist;
class PlaylistItem;
class Selection;

class InsertCommand : public Command {
 public:
								InsertCommand(Playlist* list,
											  Selection* selection,
											  PlaylistItem** const items,
											  int32 count,
											  int64 insertFrame,
											  uint32 insertTrack,
											  int32 insertIndex = -1);
	virtual						~InsertCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			Selection*			fSelection;
			PlaylistItem**		fItems;
			int32				fCount;
			int32				fInsertIndex;

			int64				fInsertFrame;
			uint32				fInsertTrack;

			bool				fItemsInserted;

			int64				fPushedBackStart;
			int64				fPushedBackFrames;
};

#endif // INSERT_COMMAND_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef MOVE_PLAYLIST_ITEMS_COMMAND_H
#define MOVE_PLAYLIST_ITEMS_COMMAND_H

#include "Command.h"

// TODO: make a templated "move items" command?

class PlaylistItem;
class Playlist;

class MovePlaylistItemsCommand : public Command {
 public:
								MovePlaylistItemsCommand(
									Playlist* playlist,
									PlaylistItem** const items,
									int32 count,
									int32 toIndex);
	virtual						~MovePlaylistItemsCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			PlaylistItem**		fItems;
			int32*				fIndices;
			int32				fToIndex;
			int32				fCount;
};

#endif // MOVE_PLAYLIST_ITEMS_COMMAND_H

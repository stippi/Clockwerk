/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DELETE_COMMAND_H
#define DELETE_COMMAND_H

#include "Command.h"

class Playlist;
class PlaylistItem;
class Selection;

class DeleteCommand : public Command {
 public:
								DeleteCommand(Playlist* list,
											  PlaylistItem** items,
											  int32 itemCount,
											  Selection* selection);
								DeleteCommand(Playlist* list,
											  const PlaylistItem** items,
											  int32 itemCount);
	virtual						~DeleteCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			Playlist*			fPlaylist;
			PlaylistItem**		fItems;
			int32*				fIndices;
			int32				fItemCount;
			bool				fItemsRemoved;
			Selection*			fSelection;
};

#endif // DELETE_COMMAND_H

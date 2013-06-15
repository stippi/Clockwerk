/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DUPLICATE_ITEMS_COMMAND_H
#define DUPLICATE_ITEMS_COMMAND_H

#include "Command.h"

class Playlist;
class PlaylistItem;
class Selection;

class DuplicateItemsCommand : public Command {
 public:
								DuplicateItemsCommand(
										Playlist* list,
										const PlaylistItem** items,
										int32 count,
										Selection* selection = NULL);
	virtual						~DuplicateItemsCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:

	struct PushBackInfo {
			PlaylistItem*		item;
			int64				insert_frame;
			int64				push_back_start;
			int64				push_back_frames;
	};

			Playlist*			fPlaylist;
			Selection*			fSelection;

			PushBackInfo*		fInfos;
			int32				fCount;

			bool				fItemsInserted;
};

#endif // DUPLICATE_ITEMS_COMMAND_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CUT_COMMAND_H
#define CUT_COMMAND_H

#include "Command.h"

class Playlist;
class PlaylistItem;

class CutCommand : public Command {
 public:
								CutCommand(PlaylistItem* item,
										   int64 cutFrame);
	virtual						~CutCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PlaylistItem*		fOriginalItem;
			PlaylistItem*		fInsertedItem;
};

#endif // CUT_COMMAND_H

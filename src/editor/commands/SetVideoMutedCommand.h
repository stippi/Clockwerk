/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SET_VIDEO_MUTED_COMMAND__H
#define SET_VIDEO_MUTED_COMMAND__H

#include "Command.h"

class PlaylistItem;

class SetVideoMutedCommand : public Command {
 public:
								SetVideoMutedCommand(PlaylistItem* item);
	virtual						~SetVideoMutedCommand();

	// Command interface
	virtual	status_t			InitCheck();

	virtual status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PlaylistItem*		fItem;
};

#endif // SET_VIDEO_MUTED_COMMAND__H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SET_AUDIO_MUTED_COMMAND_H
#define SET_AUDIO_MUTED_COMMAND__H

#include "Command.h"

class PlaylistItem;

class SetAudioMutedCommand : public Command {
 public:
								SetAudioMutedCommand(PlaylistItem* item);
	virtual						~SetAudioMutedCommand();

	// Command interface
	virtual	status_t			InitCheck();

	virtual status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PlaylistItem*		fItem;
};

#endif // SET_AUDIO_MUTED_COMMAND__H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REPLACE_CLIP_COMMAND_H
#define REPLACE_CLIP_COMMAND_H

#include "Command.h"

class Clip;
class ClipPlaylistItem;

class ReplaceClipCommand : public Command {
 public:
								ReplaceClipCommand(ClipPlaylistItem* item,
												   Clip* newClip);
	virtual						~ReplaceClipCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			ClipPlaylistItem*	fItem;

			Clip*				fOldClip;
			uint64				fOldDuration;
			Clip*				fNewClip;
};

#endif // REPLACE_CLIP_COMMAND_H

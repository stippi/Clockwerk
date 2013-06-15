/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PICK_END_COMMAND_H
#define PICK_END_COMMAND_H

#include "TrackingCommand.h"

class PlaylistItem;

class PickEndCommand : public TrackingCommand {
 public:
								PickEndCommand(PlaylistItem* item,
											   int64 dragStartFrame);
	virtual						~PickEndCommand();

	// Command interface
	virtual	status_t			InitCheck();

	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

	// TrackingCommand interface
	virtual	void				Track(BPoint where,
									  TimelineView* view,
									  SnapFrameList* snapFrames);

 private:
			void				_SetItemEndFrame(int64 endFrame);

			PlaylistItem*		fItem;

			int64				fLastFrame;
									// needed for tracking

			int64				fEndFrameOffset;
									// used to toggle the
									// end frame of the
									// item

			int64				fPushedBackStart;
			int64				fPushedBackFrames;
									// all items behind the
									// dragged item might have
									// been pushed back to make
									// room while dragging
};

#endif // PICK_END_COMMAND_H

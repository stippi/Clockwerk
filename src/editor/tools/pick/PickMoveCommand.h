/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PICK_COMMAND_H
#define PICK_COMMAND_H

#include "TrackingCommand.h"

class PlaylistItem;

class PickMoveCommand : public TrackingCommand {
 public:
								PickMoveCommand(PlaylistItem* item,
												int64 dragStartFrame);
	virtual						~PickMoveCommand();

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
			void				_SetItemPosition(int64 startFrame,
												 uint32 track);

			PlaylistItem*		fItem;

			int64				fLastFrame;
									// needed for tracking

			int64				fStartFrameOffset;
			int32				fTrackOffset;
									// used to toggle the
									// start frame of the
									// item

			int64				fPushedBackStart;
			int64				fPushedBackFrames;
									// all items behind the
									// dragged item might have
									// been pushed back to make
									// room while dragging
};

#endif // PICK_COMMAND_H

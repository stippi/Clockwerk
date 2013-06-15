/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TRACKING_COMMAND_H
#define TRACKING_COMMAND_H

#include <Point.h>

#include "Command.h"

class CurrentFrame;
class SnapFrameList;
class TimelineView;

class TrackingCommand : public Command {
 public:
								TrackingCommand();
	virtual						~TrackingCommand();

	// Command Interface
	virtual	status_t			Perform();

	// TrackingCommand
	virtual	void				Track(BPoint where,
									  TimelineView* view,
									  SnapFrameList* snapFrames) = 0;

			void				SetCurrentFrame(CurrentFrame* frame);
			void				SetCurrentFrame(int64 frame);
			void				ResetCurrentFrame();

 private:
			CurrentFrame*		fCurrentFrame;
			int64				fOriginalPlaybackPosition;

};

#endif // TRACKING_COMMAND_H

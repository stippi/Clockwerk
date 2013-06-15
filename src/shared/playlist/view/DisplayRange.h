/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DISPLAY_RANGE_H
#define DISPLAY_RANGE_H

#include "Observable.h"

class DisplayRange : public Observable {
 public:
								DisplayRange();
	virtual						~DisplayRange();

			void				SetFirstFrame(int64 frame);
			int64				FirstFrame() const
									{ return fFirstFrame; }

			void				SetLastFrame(int64 frame);
			int64				LastFrame() const
									{ return fLastFrame; }

			void				SetFrames(int64 firstFrame, int64 lastFrame);
			void				MoveBy(int64 frameOffset);

	inline	uint64				DisplayedFrames() const
									{ return fLastFrame - fFirstFrame + 1; }

 private:
			int64				fFirstFrame;
			int64				fLastFrame;
};

#endif // DISPLAY_RANGE_H

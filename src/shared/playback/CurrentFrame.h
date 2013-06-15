/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef CURRENT_FRAME_H
#define CURRENT_FRAME_H

#include "Observable.h"

class CurrentFrame : public Observable {
 public:
								CurrentFrame();
	virtual						~CurrentFrame();

			void				SetFrame(int64 frame);
			int64				Frame() const
									{ return fFrame; }

			void				SetPlaybackOffset(int64 offset);

			void				SetVirtualFrame(int64 frame);
			int64				VirtualFrame() const
									{ return fFrame - fPlaybackOffset; }

			// just a dull helper function to
			// be able to avoid setting the current frame
			// while it is being dragged with the mouse,
			// this avoids a jumpy current frame indication,
			// because the video playback is actually lagging
			// a little bit behind.
			void				SetBeingDragged(bool dragged);
			bool				BeingDragged() const
									{ return fBeingDragged; }

 private:
			int64				fFrame;
			int64				fPlaybackOffset;
			bool				fBeingDragged;
};

#endif // CURRENT_FRAME_H

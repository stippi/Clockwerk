/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CurrentFrame.h"

// constructor
CurrentFrame::CurrentFrame()
	: Observable(),
	  fFrame(0),
	  fPlaybackOffset(0),
	  fBeingDragged(false)
{
}

// destructor
CurrentFrame::~CurrentFrame()
{
}

// SetFrame
void
CurrentFrame::SetFrame(int64 frame)
{
	if (frame != fFrame) {
		fFrame = frame;
		Notify();
	}
}

// SetPlaybackOffset
void
CurrentFrame::SetPlaybackOffset(int64 offset)
{
	if (offset != fPlaybackOffset) {
		fPlaybackOffset = offset;
		Notify();
	}
}

// SetVirtualFrame
void
CurrentFrame::SetVirtualFrame(int64 frame)
{
	SetFrame(frame + fPlaybackOffset);
}

// SetBeingDragged
void
CurrentFrame::SetBeingDragged(bool dragged)
{
	if (dragged != fBeingDragged) {
		fBeingDragged = dragged;
		Notify();
	}
}

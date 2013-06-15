/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DisplayRange.h"

#include <stdio.h>

// constructor
DisplayRange::DisplayRange()
	: Observable(),
	  fFirstFrame(0),
	  fLastFrame(0)
{
}

// destructor
DisplayRange::~DisplayRange()
{
}

// SetFirstFrame
void
DisplayRange::SetFirstFrame(int64 frame)
{
	if (frame != fFirstFrame) {
		fFirstFrame = frame;
		Notify();
	}
}

// SetLastFrame
void
DisplayRange::SetLastFrame(int64 frame)
{
	if (frame != fLastFrame) {
		fLastFrame = frame;
		Notify();
	}
}

// SetFrames
void
DisplayRange::SetFrames(int64 firstFrame, int64 lastFrame)
{
	if (firstFrame != fFirstFrame || lastFrame != fLastFrame) {
		fFirstFrame = firstFrame;
		fLastFrame = lastFrame;
		Notify();
	}
}

// MoveBy
void
DisplayRange::MoveBy(int64 frameOffset)
{
	if (frameOffset != 0) {
		fFirstFrame += frameOffset;
		fLastFrame += frameOffset;
		Notify();
	}
}


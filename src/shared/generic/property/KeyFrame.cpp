/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "KeyFrame.h"

#include <new>
#include <stdio.h>

#include "Property.h"

using std::nothrow;

// constructor
KeyFrame::KeyFrame(::Property* property, int64 frame, bool locked)
	: Observable(),
	  fFrame(frame),
	  fLocked(locked),
	  fProperty(property)
{
}

// constructor
KeyFrame::KeyFrame(const KeyFrame& other)
	: Observable(),
	  fFrame(other.fFrame),
	  fLocked(other.fLocked),
	  fProperty(other.fProperty->Clone(false))
{
}

// destructor
KeyFrame::~KeyFrame()
{
	delete fProperty;
}

// SetFrame
void
KeyFrame::SetFrame(int64 frame, bool force)
{
	if (fLocked && !force)
		return;

	if (fFrame != frame) {
		fFrame = frame;
		Notify();
	}
}

// SetLocked
void
KeyFrame::SetLocked(bool locked)
{
	if (fLocked != locked) {
		fLocked = locked;
		Notify();
	}
}

// SetScale
bool
KeyFrame::SetScale(float scale)
{
	return fProperty->SetScale(scale);
}

// Scale
float
KeyFrame::Scale() const
{
	return fProperty->Scale();
}

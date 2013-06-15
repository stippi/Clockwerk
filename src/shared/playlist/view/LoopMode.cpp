/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "LoopMode.h"

#include <stdio.h>

#include "PlaybackManager.h"

// constructor
LoopMode::LoopMode()
	: Observable(),
	  fMode(LOOPING_ALL)
{
}

// destructor
LoopMode::~LoopMode()
{
}

// SetMode
void
LoopMode::SetMode(int32 mode)
{
	if (mode != fMode) {
		fMode = mode;
		Notify();
	}
}


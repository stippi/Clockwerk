/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "VideoViewSelection.h"

// constructor
VideoViewSelection::VideoViewSelection()
	: Selection(),
	  fAssociatedSelectable(NULL)
{
}

// destructor
VideoViewSelection::~VideoViewSelection()
{
}

// SetAssociatedSelectable
void
VideoViewSelection::SetAssociatedSelectable(Selectable* selectable)
{
	fAssociatedSelectable = selectable;
}

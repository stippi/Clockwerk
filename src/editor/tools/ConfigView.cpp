/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ConfigView.h"

#include <String.h>

#include "Tool.h"

// constructor
ConfigView::ConfigView(::Tool* tool)
	: BView(BRect(0, 0, 40, 20), NULL, B_FOLLOW_ALL, 0),
	  fTool(tool)
{
	BString name(tool->Name());
	name << " config view";
	SetName(name.String());
}

// destructor
ConfigView::~ConfigView()
{
}

// #pragma mark -

// UpdateStrings
void
ConfigView::UpdateStrings()
{
}

// SetActive
void
ConfigView::SetActive(bool active)
{
}

// SetEnabled
void
ConfigView::SetEnabled(bool enable)
{
}

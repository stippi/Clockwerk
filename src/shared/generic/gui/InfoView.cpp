/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "InfoView.h"

#include <ControlLook.h>


// constructor
InfoView::InfoView(BRect frame, const char* name)
	: BView(frame, name,
			B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// destructor
InfoView::~InfoView()
{
}

// Draw
void
InfoView::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawMenuBarBackground(this, bounds, updateRect, base);
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "IconBar.h"

#include <stdio.h>

// constructor
IconBar::IconBar(BRect frame, enum orientation orientation)
	: BView(frame, "icon bar", B_FOLLOW_NONE, /*B_FRAME_EVENTS |*/ B_WILL_DRAW),
	  fPreviousBounds(frame.OffsetToCopy(B_ORIGIN)),
	  fOrientation(orientation)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

// destructor
IconBar::~IconBar()
{
}

// AllAttached
void
IconBar::AllAttached()
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// FrameResized
void
IconBar::FrameResized(float width, float height)
{
	fPreviousBounds = Bounds();
}

// Draw
void
IconBar::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenMax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken1 = tint_color(base, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

	BRect r(Bounds());

	if (fOrientation == B_HORIZONTAL) {
		BeginLineArray(2);
			AddLine(BPoint(r.left, r.top + 1),
					BPoint(r.right, r.top + 1), lightenMax);
			AddLine(BPoint(r.left, r.top),
					BPoint(r.right, r.top), darken2);
		EndLineArray();
	
		r.top += 2;
	} else {
		BeginLineArray(2);
			AddLine(BPoint(r.right, r.top),
					BPoint(r.right, r.bottom), darken2);
			AddLine(BPoint(r.right - 1, r.top),
					BPoint(r.right - 1, r.bottom), darken1);
		EndLineArray();
	
		r.right -= 2;
	}

	r = r & updateRect;
	SetHighColor(base);
	FillRect(r);
}

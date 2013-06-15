/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ListLabelView.h"

#include <stdio.h>

// constructor
ListLabelView::ListLabelView(BRect frame, const char* name, uint32 followMode,
		const char* label)
	: BView(frame, name, followMode, B_WILL_DRAW | B_FRAME_EVENTS)
	, fLabel(label ? label : "")
	, fOldWidth(frame.Width())
{
	SetFont(be_bold_font);
	SetViewColor(B_TRANSPARENT_COLOR);
}

// destructor
ListLabelView::~ListLabelView()
{
}

// FrameResized
void
ListLabelView::FrameResized(float width, float height)
{
	Invalidate(BRect(min_c(fOldWidth - 1, width - 1), 0,
		max_c(fOldWidth - 1, width - 1), height));
	fOldWidth = width;
}

// Draw
void
ListLabelView::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenMax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

	BRect r(Bounds());

	BeginLineArray(3);
		AddLine(BPoint(r.left, r.bottom - 1),
				BPoint(r.left, r.top), lightenMax);
		AddLine(BPoint(r.right, r.top),
				BPoint(r.right, r.bottom), darken2);
		AddLine(BPoint(r.right - 1, r.bottom),
				BPoint(r.left, r.bottom), darken2);
	EndLineArray();

	r.left += 1;
	r.right -= 1;
	r.bottom -= 1;

	SetLowColor(base);
	FillRect(r & updateRect, B_SOLID_LOW);

	font_height fh;
	GetFontHeight(&fh);

	SetHighColor(0, 0, 0, 255);
	BPoint labelPos;
	labelPos.x = 5;
	labelPos.y = r.top + floorf((r.Height() + fh.ascent) / 2.0);
	DrawString(fLabel.String(), labelPos);
}

// GetPreferredSize
void
ListLabelView::GetPreferredSize(float* width, float* height)
{
	font_height fh;
	GetFontHeight(&fh);

	if (width)
		*width = max_c(Bounds().Width(), StringWidth(fLabel.String()) + 10);
	if (height)
		*height = fh.ascent + fh.descent + 8;
}

// #pragma mark -

// SetLabel
void
ListLabelView::SetLabel(const char* label)
{
	if (label)
		fLabel = label;
	else
		fLabel = "";

	// TODO: just label area
	Invalidate();
}



/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */


#include "PlaybackFrameView.h"

#include <stdio.h>

#include <ControlLook.h>


// constructor
PlaybackFrameView::PlaybackFrameView(int32 mode)
	:
	BView("playback frame view", B_WILL_DRAW | B_FRAME_EVENTS),
	fMode(mode),
	fFrame(0),
	fFPS(25.0)
{
	fTimeText.SetTo("0 : 00 : 00 : 01");
	SetViewColor(B_TRANSPARENT_32_BIT);
	SetFont(be_bold_font);
}

// destructor
PlaybackFrameView::~PlaybackFrameView()
{
}

// MinSize
BSize
PlaybackFrameView::MinSize()
{
	// TODO: Cache
	font_height fh;
	GetFontHeight(&fh);

	BSize size;
	size.width = StringWidth("0 : 00 : 00 : 00") + 40.0;
	size.height = fh.ascent + fh.descent + 6.0;

	return size;
}

// MaxSize
BSize
PlaybackFrameView::MaxSize()
{
	BSize size(MinSize());
	size.height = B_SIZE_UNLIMITED;
	return size;
}

// PreferredSize
BSize
PlaybackFrameView::PreferredSize()
{
	return MinSize();
}

// FrameResized
void
PlaybackFrameView::FrameResized(float width, float height)
{
	font_height fh;
	GetFontHeight(&fh);

	fTextY = (height - (ceilf(fh.ascent) + ceilf(fh.descent))) / 2.0
		+ ceilf(fh.ascent);
}

// Draw
void
PlaybackFrameView::Draw(BRect updateRect)
{
	rgb_color bg = ui_color(B_PANEL_BACKGROUND_COLOR);

	BRect r(Bounds());
	be_control_look->DrawTextControlBorder(this, r, updateRect, bg);

	SetLowColor(0, 0, 0, 255);
	SetHighColor(0, 255, 0, 255);

	float textX;
	if (fMode == TIME_MODE)
		textX = (r.Width() - StringWidth("0 : 00 : 00 : 00")) / 2.0;
	else
		textX = r.right - StringWidth(fTimeText.String()) - 20.0;
	
	FillRect(r, B_SOLID_LOW);
	DrawString(fTimeText.String(), BPoint(textX, fTextY));
}

// MouseDown
void
PlaybackFrameView::MouseDown(BPoint where)
{
	if (Bounds().Contains(where)) {
		if (fMode == TIME_MODE)
			SetMode(FRAME_MODE);
		else
			SetMode(TIME_MODE);
	}
}

// SetFramesPerSecond
void
PlaybackFrameView::SetFramesPerSecond(float fps)
{
	fFPS = fps;
	SetTime(fFrame);
}

// SetTime
void
PlaybackFrameView::SetTime(int32 frame)
{
	fFrame = frame;
	if (fMode == TIME_MODE) {
		if (fFPS >= 1.0) {
			int32 seconds = fFrame / (int32)fFPS;
			int32 minutes = seconds / 60L;
			int32 hours = minutes / 60L;
		
			fTimeText.SetTo("");
			fTimeText << hours << " : ";
			char string[2];
			sprintf(string,"%0*ld", 2, (int32)(minutes - hours * 60L));
			fTimeText << string << " : ";
			sprintf(string,"%0*ld", 2, (int32)(seconds - minutes * 60L));
			fTimeText << string << " : ";
			int32 frames = (int32)(fFrame % (int32)fFPS);
			if (frames >= 0)
				sprintf(string,"%0*ld", 2, frames + 1);
			else
				sprintf(string,"%0*ld", 2, frames);
			fTimeText << string;
		}
	} else {
		fTimeText.SetTo("");
		fTimeText << fFrame + 1;
	}
	Invalidate();
}

// SetMode
void
PlaybackFrameView::SetMode(int32 mode)
{
	fMode = mode;
	SetTime(fFrame);
}



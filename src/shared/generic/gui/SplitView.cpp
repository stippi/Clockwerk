/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "SplitView.h"

#include <Cursor.h>
#include <Message.h>
#include <Window.h>

#include <stdio.h>

const unsigned char kSplitterCursor[] = { 16, 1, 8, 8,
	0x03, 0xc0, 0x02, 0x40, 0x02, 0x40, 0x02, 0x40,
	0x1a, 0x58, 0x2a, 0x54, 0x4a, 0x52, 0x8a, 0x51,
	0x8a, 0x51, 0x4a, 0x52, 0x2a, 0x54, 0x1a, 0x58,
	0x02, 0x40, 0x02, 0x40, 0x02, 0x40, 0x03, 0xc0,

	0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0,
	0x1b, 0xd8, 0x3b, 0xdc, 0x7b, 0xde, 0xfb, 0xdf,
	0xfb, 0xdf, 0x7b, 0xde, 0x3b, 0xdc, 0x1b, 0xd8,
	0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0, 0x03, 0xc0 };

#define SPLITTER_WIDTH 5


//	#pragma mark - Splitter


Splitter::Splitter(orientation o)
	: BView(BRect(0, 0, SPLITTER_WIDTH, SPLITTER_WIDTH), "splitter",
			B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS),
	  fTracking(false),
	  fIgnoreMouseMoved(false),
	  fMousePos(B_ORIGIN),
	  fPreviousPosition(-1),
	  fOrientation(o)
{
}


Splitter::~Splitter()
{
}


void
Splitter::Draw(BRect updateRect)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);

	BRect r = Bounds();
	if (fOrientation == B_VERTICAL)
		r.bottom++;
	else
		r.right++;

	// frame
	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), light);
		if (fOrientation == B_VERTICAL) {
			AddLine(BPoint(r.left + 1.0, r.top),
					BPoint(r.right - 1, r.top), light);
			AddLine(BPoint(r.right, r.top),
					BPoint(r.right, r.bottom), shadow);
		} else {
			AddLine(BPoint(r.left + 1.0, r.top),
					BPoint(r.right, r.top), light);
			AddLine(BPoint(r.right, r.top + 1.0),
					BPoint(r.right, r.bottom), shadow);
		}
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), shadow);
	EndLineArray();

	// background frame
	r.InsetBy(1.0, 1.0);
	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), background);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), background);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), background);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), background);
	EndLineArray();

	// dots
	r.InsetBy(1.0, 1.0);
	if (fOrientation == B_VERTICAL) {
		BPoint dot = r.LeftTop();
		BPoint stop = r.LeftBottom();
		int32 num = 1;
		while (dot.y <= stop.y) {
			rgb_color col1;
			rgb_color col2;
			switch (num) {
				case 1:
					col1 = darkShadow;
					col2 = background;
					break;
				case 2:
					col1 = background;
					col2 = light;
					break;
				case 3:
					col1 = background;
					col2 = background;
					num = 0;
					break;
			}
			SetHighColor(col1);
			StrokeLine(dot, dot, B_SOLID_HIGH);
			SetHighColor(col2);
			dot.x++;
			StrokeLine(dot, dot, B_SOLID_HIGH);
			dot.x -= 1.0;
			// next pixel
			num++;
			dot.y++;
		}
	} else {
		BPoint dot = r.LeftTop();
		BPoint stop = r.RightTop();
		int32 num = 1;
		while (dot.x <= stop.x) {
			rgb_color col1;
			rgb_color col2;
			switch (num) {
				case 1:
					col1 = darkShadow;
					col2 = background;
					break;
				case 2:
					col1 = background;
					col2 = light;
					break;
				case 3:
					col1 = background;
					col2 = background;
					num = 0;
					break;
			}
			SetHighColor(col1);
			StrokeLine(dot, dot, B_SOLID_HIGH);
			SetHighColor(col2);
			dot.y++;
			StrokeLine(dot, dot, B_SOLID_HIGH);
			dot.y -= 1.0;
			// next pixel
			num++;
			dot.x++;
		}
	}
}


void
Splitter::MouseDown(BPoint where)
{
	int32 clicks = 1;
	if (Window() != NULL && Window()->CurrentMessage() != NULL)
		Window()->CurrentMessage()->FindInt32("clicks", &clicks);
	if (clicks == 2) {
		DoubleClickMove();
		return;
	}

	fTracking = true;
	fMousePos = where;
	ConvertToParent(&fMousePos);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);

	fInitialPosition = fOrientation == B_VERTICAL
		? Frame().left : Frame().top;
}


void
Splitter::MouseUp(BPoint where)
{
	fTracking = false;
}


void
Splitter::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	if (transit == B_ENTERED_VIEW) {
		BCursor cursor(kSplitterCursor);
		SetViewCursor(&cursor);
	}

	if (!fTracking || fIgnoreMouseMoved) {
		fIgnoreMouseMoved = false;
		return;
	}

	// stupid R5 app_server sends us bogus mouse
	// coordinates when we have moved meanwhile
	// (by relayouting)
	uint32 buttons;
	GetMouse(&where, &buttons, false);
	ConvertToParent(&where);

	float offset;
	if (fOrientation == B_VERTICAL)
		offset = where.x - fMousePos.x;
	else
		offset = where.y - fMousePos.y;

	if (offset != 0.0)
		Move(offset);
}


void
Splitter::FrameMoved(BPoint parentPoint)
{
	fIgnoreMouseMoved = true;
}


void
Splitter::Move(float offset)
{
	SplitView* parent = dynamic_cast<SplitView*>(Parent());
	if (parent == NULL)
		return;

	parent->SetSplitterPosition(fInitialPosition + offset);
}


void
Splitter::DoubleClickMove()
{
	SplitView* parent = dynamic_cast<SplitView*>(Parent());
	if (parent == NULL)
		return;

	// double click - on first time, jump to the closer border, on
	// the second time, jump back to the previous position

	float size = (fOrientation == B_VERTICAL
		? parent->Bounds().Width() : parent->Bounds().Height())
		- SPLITTER_WIDTH;
	float min = parent->SplitterMinimum();
	float max = parent->SplitterMaximum();
	if (parent->ProportionalMode()) {
		min *= size;
		max *= size;
	}
	max = min_c(max, size);

	float position = parent->SplitterPosition();
	if (min != position && max != position || fPreviousPosition == -1) {
		fPreviousPosition = position;

		float delta = (max - min) / 2 + min - position;
		if (delta <= 0.0f)
			parent->SetSplitterProportion(1.0f);
		else
			parent->SetSplitterProportion(0.0f);
	} else
		parent->SetSplitterPosition(fPreviousPosition);
}


void
Splitter::SetOrientation(orientation o)
{
	fOrientation = o;
}


//	#pragma mark - SplitView


SplitView::SplitView(BRect frame, const char* name, orientation o,
		bool proportional, uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP)
	: BView(frame, name, B_FRAME_EVENTS, resizingMode),
	fOrientation(o),
	fProportional(proportional),
	fMin(0),
	fMax(32767.0),
	fPosition(-1),
	fProportion(0.5),
	fSplitter(NULL)
{
}


SplitView::~SplitView()
{
}


float
SplitView::_Size() const
{
	return (Orientation() == B_VERTICAL
		? Bounds().Width() : Bounds().Height()) - SPLITTER_WIDTH;
}


void
SplitView::AllAttached()
{
	fPreviousSize = _Size();
	Relayout();
}


void
SplitView::FrameResized(float width, float height)
{
	if (fProportional && fPosition != -1) {
		fProportion = fPosition / fPreviousSize;
		fPosition = -1;
	}
	fPreviousSize = _Size();

	Relayout();
}


void
SplitView::SetOrientation(orientation o)
{
	if (o == fOrientation)
		return;

	fOrientation = o;
	Relayout();
}


orientation
SplitView::Orientation() const
{
	return fOrientation;
}


void
SplitView::SetProportionalMode(bool proportional)
{
	if (fProportional == proportional)
		return;

	fProportional = proportional;
	Relayout();
}


bool
SplitView::ProportionalMode() const
{
	return fProportional;
}


void
SplitView::SetSplitterProportion(float proportion)
{
	// make sure 'proportion' is within valid bounds
	float min = fMin;
	float max = fMax;
	if (!ProportionalMode()) {
		min /= _Size();
		max /= _Size();
	}
	if (proportion < min)
		proportion = min;
	else if (proportion > max)
		proportion = max;

	fProportion = proportion;
	fPosition = -1;
	Relayout();
}


float
SplitView::SplitterProportion() const
{
	return fProportion;
}


void
SplitView::SetSplitterPosition(float position)
{
	// make sure 'position' is within valid bounds
	if (fProportional) {
		float size = _Size();
		if (position < size * fMin)
			position = size * fMin;
		else if (position > size * fMax)
			position = size * fMax;
	} else {
		if (position < fMin)
			position = fMin;
		else if (position > fMax)
			position = fMax;
	}

	if (position == fPosition)
		return;

	fPosition = position;
	fProportion = fPosition / _Size();
	Relayout();
}


float
SplitView::SplitterPosition() const
{
	return fPosition;
}


void
SplitView::SetSplitterLimits(float min, float max)
{
	if (min < 0.0f)
		min = 0.0f;
	if (fProportional && max > 1.0f)
		max = 1.0f;

	fMin = min;
	fMax = max;

	SetSplitterPosition(fPosition);
}


float
SplitView::SplitterMinimum() const
{
	return fMin;
}


float
SplitView::SplitterMaximum() const
{
	return fMax;
}


Splitter*
SplitView::NewSplitter() const
{
	return new Splitter(Orientation());
}


void
SplitView::Relayout()
{
	if (fSplitter)
		fSplitter->SetOrientation(Orientation());

	float max = _Size();

	if (fPosition == -1) {
		// set initial default position in the middle
		fPosition = max * fProportion;
	}
	if (fPosition > max)
		fPosition = max;

	int32 count = CountChildren();
	float last = 0;

	for (int32 index = 0; index < count; index++) {
		BView* child = ChildAt(index);
		float size;
		if (index == 0)
			size = fPosition;
		else if (Orientation() == B_VERTICAL)
			size = (Bounds().right + 1 - last) / (count - index);
		else
			size = (Bounds().bottom + 1 - last) / (count - index);

		if (index == 1 && fSplitter == NULL) {
			// we need to add a splitter
			AddChild(fSplitter = NewSplitter(), child);
			child = fSplitter;
			count++;
		}

		if (child == fSplitter)
			size = SPLITTER_WIDTH;// + (Orientation() == B_VERTICAL ? 0 : 1);

		if (Orientation() == B_VERTICAL) {
			child->MoveTo(last, 0);
			child->ResizeTo(size, Bounds().Height());
		} else {
			child->MoveTo(0, last);
			child->ResizeTo(Bounds().Width(), size);
		}

		last += size + 1;
	}
}


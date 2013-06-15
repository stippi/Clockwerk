/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleTopView.h"

#include <stdio.h>

#include <Cursor.h>

#include "cursors.h"

// Splitter
class ScheduleTopView::Splitter : public BView {
 public:
								Splitter(ScheduleTopView* parent);
	virtual						~Splitter();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	virtual	void				FrameMoved(BPoint parentPoint);
	virtual	void				FrameResized(float width, float height);
	virtual	void				GetPreferredSize(float* width,
									float* height);

	// Splitter
	virtual	void				Move(float offset) = 0;

			void				SetBorderSizes(uint8 left, uint8 top,
									uint8 right, uint8 bottom);

 private:
			bool				fTracking;
			bool				fIgnoreMouseMoved;
			BPoint				fMousePos;

			uint8				fLeftBorderSize;
			uint8				fTopBorderSize;
			uint8				fRightBorderSize;
			uint8				fBottomBorderSize;
			float				fOldSize;

 protected:
			ScheduleTopView*	fParent;
			orientation			fOrient;
};

// ListSplitter
class ScheduleTopView::ListSplitter : public ScheduleTopView::Splitter {
 public:
								ListSplitter(ScheduleTopView* parent);
	virtual						~ListSplitter();

	// Splitter interface
	virtual	void				MouseDown(BPoint where);
	virtual	void				Move(float offset);

	// ListSplitter
			void				SetListAreaWidth(int32 width);
 private:
			int32				fListAreaWidth;
			float				fInitialProportion;
};

// #pragma mark - Splitter

// constructor
ScheduleTopView::Splitter::Splitter(ScheduleTopView* parent)
	: BView(BRect(0, 0, 10, 10), "splitter", B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS)
	, fTracking(false)
	, fIgnoreMouseMoved(false)
	, fMousePos(B_ORIGIN)

	, fLeftBorderSize(1)
	, fTopBorderSize(1)
	, fRightBorderSize(1)
	, fBottomBorderSize(1)

	, fOldSize(10)

	, fParent(parent)
	, fOrient(B_HORIZONTAL)
{
}

// destructor
ScheduleTopView::Splitter::~Splitter()
{
}

// Draw
void
ScheduleTopView::Splitter::Draw(BRect updateRect)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color light = tint_color(background, B_LIGHTEN_MAX_TINT);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color darkShadow = tint_color(background, B_DARKEN_3_TINT);

	BRect r = Bounds();

	// frame
	BeginLineArray(fLeftBorderSize + fTopBorderSize
		+ fRightBorderSize + fBottomBorderSize);

		// outer edges
		// left
		if (fLeftBorderSize > 1) {
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.top), shadow);
			r.left += 1;
		}
		// top
		if (fTopBorderSize > 1) {
			AddLine(BPoint(r.left, r.top),
					BPoint(r.right, r.top), shadow);
			r.top += 1;
		}
		// right
		if (fRightBorderSize > 1) {
			AddLine(BPoint(r.right, r.top),
					BPoint(r.right, r.bottom), shadow);
			r.right -= 1;
		}
		// bottom
		if (fBottomBorderSize > 1) {
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.right, r.bottom), shadow);
			r.bottom -= 1;
		}

		// left
		if (fLeftBorderSize > 0) {
			AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.top), light);
			r.left += 1;
		}
		if (fOrient == B_VERTICAL) {
			if (fRightBorderSize > 0) {
				AddLine(BPoint(r.right, r.top),
						BPoint(r.right, r.bottom), shadow);
				r.right -= 1;
			}
			if (fTopBorderSize > 0) {
				AddLine(BPoint(r.left, r.top),
						BPoint(r.right, r.top), light);
				r.top += 1;
			}
		} else {
			if (fTopBorderSize > 0) {
				AddLine(BPoint(r.left, r.top),
						BPoint(r.right, r.top), light);
				r.top += 1;
			}
			if (fRightBorderSize > 0) {
				AddLine(BPoint(r.right, r.top),
						BPoint(r.right, r.bottom), shadow);
				r.right -= 1;
			}
		}
		if (fBottomBorderSize > 0) {
			AddLine(BPoint(r.right, r.bottom),
					BPoint(r.left, r.bottom), shadow);
			r.bottom -= 1;
		}

	EndLineArray();

	// background frame
	SetHighColor(background);
	StrokeRect(r);

	// dots
	r.InsetBy(1.0, 1.0);
	if (fOrient == B_VERTICAL) {
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

// MouseDown
void
ScheduleTopView::Splitter::MouseDown(BPoint where)
{
	fTracking = true;
	fMousePos = where;
	ConvertToParent(&fMousePos);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

// MouseUp
void
ScheduleTopView::Splitter::MouseUp(BPoint where)
{
	fTracking = false;
}

// MouseMoved
void
ScheduleTopView::Splitter::MouseMoved(BPoint where, uint32 transit,
							  const BMessage* dragMessage)
{
	if (transit == B_ENTERED_VIEW) {
		BCursor cursor(kHSplitterCursor);
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
	if (fOrient == B_VERTICAL)
		offset = where.x - fMousePos.x;
	else
		offset = where.y - fMousePos.y;

	if (offset != 0.0)
		Move(offset);
}

// FrameMoved
void
ScheduleTopView::Splitter::FrameMoved(BPoint parentPoint)
{
	fIgnoreMouseMoved = true;
}

// FrameResized
void
ScheduleTopView::Splitter::FrameResized(float width, float height)
{
	if (fOrient == B_HORIZONTAL) {
		Invalidate(BRect(min_c(fOldSize - fRightBorderSize,
			width - fRightBorderSize), 0, max_c(fOldSize - fRightBorderSize,
			width - fRightBorderSize), height));
		fOldSize = width;
	} else {
		Invalidate(BRect(0, min_c(fOldSize - fBottomBorderSize,
			height - fBottomBorderSize), width,
			max_c(fOldSize - fBottomBorderSize, height - fBottomBorderSize)));
		fOldSize = height;
	}
}

// GetPreferredSize
void
ScheduleTopView::Splitter::GetPreferredSize(float* width, float* height)
{
	if (width) {
		*width = 4 + fLeftBorderSize + fRightBorderSize - 1;
		if (fOrient == B_HORIZONTAL)
			*width = max_c(Bounds().Width(), *width);
	}
	if (height) {
		*height = 4 + fTopBorderSize + fBottomBorderSize - 1;
		if (fOrient == B_VERTICAL)
			*height = max_c(Bounds().Height(), *height);
	}
}

// SetBorderSizes
void
ScheduleTopView::Splitter::SetBorderSizes(uint8 left, uint8 top,
	uint8 right, uint8 bottom)
{
	fLeftBorderSize = min_c(left, 2);
	fTopBorderSize = min_c(top, 2);
	fRightBorderSize = min_c(right, 2);
	fBottomBorderSize = min_c(bottom, 2);
}

// #pragma mark - ListSplitter

// constructor
ScheduleTopView::ListSplitter::ListSplitter(ScheduleTopView* parent)
	: Splitter(parent),
	  fListAreaWidth(100)
{
}

// destructor
ScheduleTopView::ListSplitter::~ListSplitter()
{
}

// MouseDown
void
ScheduleTopView::ListSplitter::MouseDown(BPoint where)
{
	fInitialProportion = fParent->ListProportion();
	Splitter::MouseDown(where);
}

// Move
void
ScheduleTopView::ListSplitter::Move(float offset)
{
	float pos = fListAreaWidth * fInitialProportion;
	pos += offset;
	float proportion = max_c(0.3, min_c(0.7, pos / fListAreaWidth));

	fParent->SetListProportion(proportion);
}

// SetListAreaWidth
void
ScheduleTopView::ListSplitter::SetListAreaWidth(int32 width)
{
	fListAreaWidth = width;
}

// #pragma mark - ScheduleTopView

// constructor
ScheduleTopView::ScheduleTopView(BRect frame)
	: BView(frame, "schedule top view", B_FOLLOW_ALL, B_FRAME_EVENTS),

	  fMenuBar(NULL),
	  fScheduleListGroup(NULL),
	  fPlaylistListGroup(NULL),
	  fScheduleGroup(NULL),

	  fListSplitter(new ListSplitter(this)),

	  fListProportion(0.5)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fListSplitter->SetBorderSizes(1, 2, 1, 0);
	fListSplitter->ResizeToPreferred();
	AddChild(fListSplitter);
}

// destructor
ScheduleTopView::~ScheduleTopView()
{
}

// AttachedToWindow
void
ScheduleTopView::AttachedToWindow()
{
	_Relayout();
}

// FrameResized
void
ScheduleTopView::FrameResized(float width, float height)
{
	_Relayout();
}

// #pragma mark -

// AddMenuBar
void
ScheduleTopView::AddMenuBar(BView* view)
{
	fMenuBar = view;
	_AddChild(view);
}

// AddStatusBar
void
ScheduleTopView::AddStatusBar(BView* view)
{
	fStatusBar = view;
	_AddChild(view);
}

// AddScheduleListGroup
void
ScheduleTopView::AddScheduleListGroup(BView* view)
{
	fScheduleListGroup = view;
	_AddChild(view);
}

// AddPlaylistListGroup
void
ScheduleTopView::AddPlaylistListGroup(BView* view)
{
	fPlaylistListGroup = view;
	_AddChild(view);
}

// AddPropertyGroup
void
ScheduleTopView::AddPropertyGroup(BView* view)
{
	fPropertyGroup = view;
	_AddChild(view);
}

// AddScheduleGroup
void
ScheduleTopView::AddScheduleGroup(BView* view)
{
	fScheduleGroup = view;
	_AddChild(view);
}

// #pragma mark -

// SetListProportion
void
ScheduleTopView::SetListProportion(float proportion)
{
	if (fListProportion == proportion)
		return;

	fListProportion = proportion;

	_Relayout();
}

// #pragma mark -

// _AddChild
void
ScheduleTopView::_AddChild(BView* view)
{
	AddChild(view);
	view->SetResizingMode(B_FOLLOW_NONE);
}

// _Relayout
void
ScheduleTopView::_Relayout()
{
	if (!fMenuBar || !fScheduleListGroup || !fPlaylistListGroup
		|| !fScheduleGroup)
		return;

	BRect bounds = Bounds();
	float listWidth = max_c(150, ceilf(bounds.Width() * 0.3));
	float propertyWidth = max_c(150, ceilf(bounds.Width() * 0.2));
	float menuBarWidth;
	float menuBarHeight;
	fMenuBar->GetPreferredSize(&menuBarWidth, &menuBarHeight);
	float listHeight = bounds.Height() - menuBarHeight;
	float splitterHeight = fListSplitter->Bounds().Height();
	float statusBarHeight;
	fStatusBar->GetPreferredSize(NULL, &statusBarHeight);

	// menu bar
	BRect frame = bounds;
	frame.bottom = frame.top + menuBarHeight;
	fMenuBar->MoveTo(frame.LeftTop());
	fMenuBar->ResizeTo(frame.Width(), frame.Height());

	// status bar	
	frame.top = frame.bottom + 1;
	frame.bottom = frame.top + statusBarHeight;
	fStatusBar->MoveTo(frame.LeftTop());
	fStatusBar->ResizeTo(frame.Width(), frame.Height());

	// schedule object list group
	frame.top = frame.bottom + 1;
	frame.bottom = frame.top + floorf(listHeight * fListProportion + 0.5)
		- splitterHeight;
	frame.right = frame.left + listWidth - 1;
//	frame.right += SPLITTER_WIDTH + 1;
	fScheduleListGroup->MoveTo(frame.LeftTop());
	fScheduleListGroup->ResizeTo(frame.Width(), frame.Height());

	// list splitter
	frame.top = frame.bottom + 1;
	frame.bottom = frame.top + splitterHeight;
	fListSplitter->MoveTo(frame.LeftTop());
	fListSplitter->ResizeTo(frame.Width(), frame.Height());

	// playlist object list group
	frame.top = frame.bottom + 1;
	frame.bottom = bounds.bottom;
	fPlaylistListGroup->MoveTo(frame.LeftTop());
	fPlaylistListGroup->ResizeTo(frame.Width(), frame.Height());

	fListSplitter->SetListAreaWidth((int32)listHeight);

	// property group
	frame.left = bounds.left + listWidth;
	frame.right = frame.left + propertyWidth - 1;
	frame.top = bounds.top + statusBarHeight + 1 + menuBarHeight + 1;
	frame.bottom = bounds.bottom;
	fPropertyGroup->MoveTo(frame.LeftTop());
	fPropertyGroup->ResizeTo(frame.Width(), frame.Height());

	// schedule group
	frame.left = bounds.left + listWidth + propertyWidth;
	frame.right = bounds.right;
	fScheduleGroup->MoveTo(frame.LeftTop());
	fScheduleGroup->ResizeTo(frame.Width(), frame.Height());
}




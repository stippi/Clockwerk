/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TopView.h"

#include <stdio.h>

#include <ControlLook.h>
#include <Cursor.h>
#include <ScrollBar.h>
#include <SplitView.h>

#include "EditorVideoView.h"

#define SPLITTER_WIDTH 5

// Splitter
class TopView::Splitter : public BView {
 public:
								Splitter(TopView* parent);
	virtual						~Splitter();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	virtual	void				FrameMoved(BPoint parentPoint);

	// Splitter
	virtual	void				Move(float offset) = 0;

 private:
			bool				fTracking;
			bool				fIgnoreMouseMoved;
			BPoint				fMousePos;

 protected:
			TopView*			fParent;
			orientation			fOrient;
};

// ListSplitter
class TopView::ListSplitter : public TopView::Splitter {
 public:
								ListSplitter(TopView* parent);
	virtual						~ListSplitter();

	// Splitter interface
	virtual	void				MouseDown(BPoint where);
	virtual	void				Move(float offset);

	// ListSplitter
			void				SetListAreaWidth(int32 width);
			void				SetMinClipProportion(float proportion);
 private:
			int32				fListAreaWidth;
			float				fInitialProportion;
			float				fMinClipProportion;
};

// VideoSplitter
class TopView::VideoSplitter : public TopView::Splitter {
 public:
								VideoSplitter(TopView* parent);
	virtual						~VideoSplitter();

	// Splitter interface
	virtual	void				MouseDown(BPoint where);
	virtual	void				Move(float offset);

	// VideoSplitter
			void				SetVideoWidth(int32 width);
			void				SetMaxVideoScale(float scale);
 private:
			int32				fVideoWidth;
			float				fInitialScale;
			float				fMaxVideoScale;
};

// ListSplitter
class TopView::TrackSplitter : public TopView::Splitter {
 public:
								TrackSplitter(TopView* parent);
	virtual						~TrackSplitter();

	// Splitter interface
	virtual	void				MouseDown(BPoint where);
	virtual	void				Move(float offset);

	// ListSplitter
			void				SetTrackAreaWidth(int32 width);
 private:
			int32				fTrackAreaWidth;
			float				fInitialProportion;
};

// #pragma mark - Splitter

// constructor
TopView::Splitter::Splitter(TopView* parent)
	: BView(BRect(0, 0, SPLITTER_WIDTH, 100), "splitter", B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS),
	  fTracking(false),
	  fIgnoreMouseMoved(false),
	  fMousePos(B_ORIGIN),
	  fParent(parent),
	  fOrient(B_VERTICAL)
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// destructor
TopView::Splitter::~Splitter()
{
}

// Draw
void
TopView::Splitter::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 flags = 0;
	if (fTracking)
		flags |= BControlLook::B_CLICKED;
	be_control_look->DrawSplitter(this, bounds, updateRect, base, B_HORIZONTAL,
		flags, 0);
}

// MouseDown
void
TopView::Splitter::MouseDown(BPoint where)
{
	fTracking = true;
	fMousePos = where;
	ConvertToParent(&fMousePos);
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	Invalidate();
}

// MouseUp
void
TopView::Splitter::MouseUp(BPoint where)
{
	fTracking = false;
	Invalidate();
}

// MouseMoved
void
TopView::Splitter::MouseMoved(BPoint where, uint32 transit,
							  const BMessage* dragMessage)
{
	if (transit == B_ENTERED_VIEW) {
		BCursor cursor(B_CURSOR_ID_RESIZE_EAST_WEST);
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

	float offset = where.x - fMousePos.x;
	if (offset != 0.0)
		Move(offset);
}

// FrameMoved
void
TopView::Splitter::FrameMoved(BPoint parentPoint)
{
	fIgnoreMouseMoved = true;
}

// #pragma mark - ListSplitter

// constructor
TopView::ListSplitter::ListSplitter(TopView* parent)
	:
	Splitter(parent),
	fListAreaWidth(100),
	fMinClipProportion(0.3f)
{
}

// destructor
TopView::ListSplitter::~ListSplitter()
{
}

// MouseDown
void
TopView::ListSplitter::MouseDown(BPoint where)
{
	fInitialProportion = fParent->ListProportion();
	Splitter::MouseDown(where);
}

// Move
void
TopView::ListSplitter::Move(float offset)
{
	float pos = fListAreaWidth * fInitialProportion;
	pos += offset;
	float maxProportion = fMinClipProportion
		+ (1.0f - fMinClipProportion) / 2.0f;
	float proportion = max_c(fMinClipProportion,
		min_c(maxProportion, pos / fListAreaWidth));

	fParent->SetListProportion(proportion);
}

// SetListAreaWidth
void
TopView::ListSplitter::SetListAreaWidth(int32 width)
{
	fListAreaWidth = width;
}

// SetMinClipProportion
void
TopView::ListSplitter::SetMinClipProportion(float proportion)
{
	fMinClipProportion = proportion;
}

// #pragma mark - VideoSplitter

// constructor
TopView::VideoSplitter::VideoSplitter(TopView* parent)
	:
	Splitter(parent),
	fVideoWidth(512),
	fMaxVideoScale(2.0f)
{
}

// destructor
TopView::VideoSplitter::~VideoSplitter()
{
}

// MouseDown
void
TopView::VideoSplitter::MouseDown(BPoint where)
{
	fInitialScale = fParent->VideoScale();
	Splitter::MouseDown(where);
}

// Move
void
TopView::VideoSplitter::Move(float offset)
{
	float width = fVideoWidth * fInitialScale;
	width -= offset;
	float scale = max_c(0.1, min_c(fMaxVideoScale, width / fVideoWidth));

	// snap to 100%
	if (fabs(scale - 1.0) < 0.03)
		scale = 1.0;

	fParent->SetVideoScale(scale);
}

// SetVideoWidth
void
TopView::VideoSplitter::SetVideoWidth(int32 width)
{
	fVideoWidth = width;
}

// SetMinListScale
void
TopView::VideoSplitter::SetMaxVideoScale(float scale)
{
	fMaxVideoScale = scale;
}

// #pragma mark - TrackSplitter

// constructor
TopView::TrackSplitter::TrackSplitter(TopView* parent)
	: Splitter(parent),
	  fTrackAreaWidth(700)
{
}

// destructor
TopView::TrackSplitter::~TrackSplitter()
{
}

// MouseDown
void
TopView::TrackSplitter::MouseDown(BPoint where)
{
	fInitialProportion = fParent->TrackProportion();
	Splitter::MouseDown(where);
}

// Move
void
TopView::TrackSplitter::Move(float offset)
{
	float pos = fTrackAreaWidth * fInitialProportion;
	pos += offset;
	float proportion = max_c(0.0, min_c(0.2, pos / fTrackAreaWidth));

	fParent->SetTrackProportion(proportion);
}

// SetVideoWidth
void
TopView::TrackSplitter::SetTrackAreaWidth(int32 width)
{
	fTrackAreaWidth = width;
}

// #pragma mark - TopView

// constructor
TopView::TopView(BRect frame)
	:
	BView(frame, "top view", B_FOLLOW_ALL, B_FRAME_EVENTS),

	fMenuBar(NULL),
	fClipListGroup(NULL),
	fPropertyListGroup(NULL),
	fIconBar(NULL),
	fStageIconBar(NULL),
	fTimelineGroup(NULL),
	fTrackGroup(NULL),
	fVideoScrollView(NULL),
	fVideoView(NULL),
	fTransportGroup(NULL),

	fListGroup(new BSplitView(B_HORIZONTAL, 0.0f)),
	fListSplitter(new ListSplitter(this)),
	fVideoSplitter(new VideoSplitter(this)),
	fTrackSplitter(new TrackSplitter(this)),

	fNativeVideoWidth(684),
	fVideoAspect(1.6),

	fVideoScale(0.7),
	fListProportion(0.5),
	fTrackProportion(0.12),

	fOriginalIconBarWidth(250)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fListGroup->SetCollapsible(false);
	AddChild(fListGroup);
	AddChild(fVideoSplitter);
	AddChild(fTrackSplitter);
}

// destructor
TopView::~TopView()
{
}

// FrameResized
void
TopView::FrameResized(float width, float height)
{
	_Relayout();
}

// #pragma mark -

// AddMenuBar
void
TopView::AddMenuBar(BView* view)
{
	fMenuBar = view;
	_AddChild(view);
}

// AddClipListGroup
void
TopView::AddClipListGroup(BView* view)
{
	fClipListGroup = view;
	fListGroup->AddChild(view, 0.5f);
}

// AddPropertyListGroup
void
TopView::AddPropertyListGroup(BView* view)
{
	fPropertyListGroup = view;
	fListGroup->AddChild(view, 0.5f);
}

// AddIconBar
void
TopView::AddIconBar(BView* view)
{
	fIconBar = view;
	fOriginalIconBarWidth = fIconBar->Bounds().Width();
	_AddChild(view);
}

// AddStageIconBar
void
TopView::AddStageIconBar(BView* view)
{
	fStageIconBar = view;
	_AddChild(view);
}

// AddTimelineGroup
void
TopView::AddTimelineGroup(BView* view)
{
	fTimelineGroup = view;
	_AddChild(view);
}

// AddTrackGroup
void
TopView::AddTrackGroup(BView* view)
{
	fTrackGroup = view;
	_AddChild(view);
}

// AddVideoView
void
TopView::AddVideoView(EditorVideoView* view, BView* container)
{
	fVideoScrollView = container;
	fVideoView = view;
	_AddChild(container);
}

// AddTransportGroup
void
TopView::AddTransportGroup(BView* view)
{
	fTransportGroup = view;
	_AddChild(view);
}

// #pragma mark -

// SetVideoSize
void
TopView::SetVideoSize(uint32 nativeWidth, float aspect)
{
	fNativeVideoWidth = nativeWidth;
	fVideoAspect = aspect;

	_Relayout();
}

// SetVideoScale
void
TopView::SetVideoScale(float scale)
{
	float minScale, maxScale;
	fVideoView->GetOverlayScaleLimits(&minScale, &maxScale);
	minScale = max_c(minScale, 0.25);
	maxScale = min_c(maxScale, 2.0);

	scale = max_c(minScale, min_c(maxScale, scale));

	if (fVideoScale == scale)
		return;

	fVideoScale = scale;

	_Relayout();
}

// SetListProportion
void
TopView::SetListProportion(float proportion)
{
	if (fListProportion == proportion)
		return;

	fListProportion = proportion;

	_Relayout();
}

// SetTrackProportion
void
TopView::SetTrackProportion(float proportion)
{
	if (proportion < 0.08)
		proportion = 0.0;
	else if (proportion < 0.1)
		proportion = 0.1;

	if (fTrackProportion == proportion)
		return;

	fTrackProportion = proportion;

	_Relayout();
}

// #pragma mark -

// _AddChild
void
TopView::_AddChild(BView* view)
{
	AddChild(view);
	view->SetResizingMode(B_FOLLOW_NONE);
}

// _Relayout
void
TopView::_Relayout()
{
	if (!fMenuBar || !fClipListGroup || !fPropertyListGroup
		|| !fIconBar || !fTimelineGroup || !fVideoView)
		return;

	BRect bounds = Bounds();
	float transportGroupWidth;
	float iconBarHeight = fIconBar->Frame().Height();
	float iconBarWidth = fStageIconBar->Frame().Width();
	float menuBarWidth, menuBarHeight;
	fMenuBar->GetPreferredSize(&menuBarWidth, &menuBarHeight);
	menuBarWidth += 50; // Grrr!
	menuBarWidth = max_c(fOriginalIconBarWidth, menuBarWidth);

	fTransportGroup->GetPreferredSize(&transportGroupWidth, NULL);

	float maxVideoWidthScale = min_c(fVideoScale,
		(bounds.Width() - menuBarWidth)
			/ (fNativeVideoWidth + B_V_SCROLL_BAR_WIDTH));
	float nativeVideoHeight = fNativeVideoWidth / fVideoAspect;
	float maxVideoHeightScale = min_c(fVideoScale,
		(bounds.Height() - 250) / (nativeVideoHeight + B_H_SCROLL_BAR_HEIGHT));
	float videoScale = min_c(maxVideoWidthScale, maxVideoHeightScale);

	// video view
	BRect videoFrame = bounds;
	float videoWidth = floorf(fNativeVideoWidth * videoScale + 0.5)
		+ B_V_SCROLL_BAR_WIDTH - 1;
	float videoHeight = floorf(nativeVideoHeight * videoScale + 0.5)
		+ B_H_SCROLL_BAR_HEIGHT - 1;
	videoFrame.left = videoFrame.right - videoWidth;
	videoFrame.bottom = videoFrame.top + videoHeight;

	fVideoScrollView->MoveTo(videoFrame.LeftTop());
	fVideoScrollView->ResizeTo(videoFrame.Width(), videoFrame.Height());

	float timelineIconBarWidth = max_c(fOriginalIconBarWidth,
		bounds.Width() - videoWidth - 8);
	transportGroupWidth = max_c(bounds.Width() - timelineIconBarWidth - 1,
		transportGroupWidth);

	// transport group
	fTransportGroup->MoveTo(bounds.right - transportGroupWidth,
		videoFrame.bottom + 1);
	fTransportGroup->ResizeTo(transportGroupWidth, iconBarHeight);

	// stage menu bar
	fStageIconBar->MoveTo(videoFrame.left - iconBarWidth - 1, videoFrame.top);
	fStageIconBar->ResizeTo(iconBarWidth, videoFrame.Height());

	// menu bar
	BRect frame = bounds;
	frame.right = videoFrame.left - SPLITTER_WIDTH - iconBarWidth - 3;
	frame.bottom = frame.top + menuBarHeight;
	fMenuBar->MoveTo(bounds.LeftTop());
	fMenuBar->ResizeTo(frame.Width(), frame.Height());

	// clip and property list group
	frame.top = frame.bottom + 1;
	frame.bottom = videoFrame.bottom;
	frame.right = videoFrame.left - iconBarWidth - 3 - SPLITTER_WIDTH;

	fListGroup->MoveTo(frame.LeftTop());
	fListGroup->ResizeTo(frame.Width(), frame.Height());

	// video splitter
	frame.left = frame.right + 1;
	frame.right = frame.left + SPLITTER_WIDTH;
	frame.top = videoFrame.top;
	fVideoSplitter->MoveTo(frame.LeftTop());
	fVideoSplitter->ResizeTo(frame.Width(), frame.Height());

	fVideoSplitter->SetVideoWidth(fNativeVideoWidth);
	float maxVideoScale = (bounds.Width() - fListGroup->MinSize().width)
		/ fNativeVideoWidth;
	fVideoSplitter->SetMaxVideoScale(maxVideoScale);

	// icon bar
	frame.left = bounds.left;
	frame.top = videoFrame.bottom + 1;
	frame.bottom = frame.top + iconBarHeight;
	frame.right = bounds.right - transportGroupWidth - 1;
	fIconBar->MoveTo(frame.LeftTop());
	fIconBar->ResizeTo(frame.Width(), frame.Height());

	int32 trackAreaWidth = (int32)bounds.right;
	int32 trackWidth = (int32)floorf(trackAreaWidth * fTrackProportion);

	// track group
	frame.left = 0;
	frame.top = frame.bottom + 1;
	frame.right = trackWidth - 1;
	frame.bottom = bounds.bottom;
	fTrackGroup->MoveTo(frame.LeftTop());
	fTrackGroup->ResizeTo(frame.Width(), frame.Height());

	// track splitter
	frame.left = trackWidth;
	frame.right = frame.left + SPLITTER_WIDTH;
	fTrackSplitter->MoveTo(frame.LeftTop());
	fTrackSplitter->ResizeTo(frame.Width(), frame.Height());

	fTrackSplitter->SetTrackAreaWidth(trackAreaWidth);

	// timeline group
	frame.left = trackWidth + SPLITTER_WIDTH + 1;
	frame.right = videoFrame.right;
	fTimelineGroup->MoveTo(frame.LeftTop());
	fTimelineGroup->ResizeTo(frame.Width(), frame.Height());
}




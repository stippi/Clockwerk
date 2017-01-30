/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TimeView.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Region.h>
#include <String.h>

#include "ui_defines.h"
#include "support.h"

#include "CommandStack.h"
#include "CurrentFrame.h"
#include "DisplayRange.h"
#include "KeyFrame.h"
#include "LoopMode.h"
#include "ModifyKeyFrameCommand.h"
#include "PlaybackManager.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "RemoveKeyFrameCommand.h"
#include "TimelineView.h"

#define MARK_HEIGHT 3.0

enum {
	TRACKING_NONE = 0,
	TRACKING_CURRENT_FRAME,
	TRACKING_KEYFRAME,
	TRACKING_PLAYBACK_START,
	TRACKING_PLAYBACK_END,
	TRACKING_PLAYBACK_RANGE,
};

enum {
	MSG_AUTOSCROLL_PULSE	= 'aspl',
};

// constructor
TimeView::TimeView(CurrentFrame* frame)
	: BView("time view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  Observer(),
	  SelectedPropertyListener(),

	  fCurrentFrame(frame),
	  fMarkerPos(fCurrentFrame->VirtualFrame()),
	  fTracking(TRACKING_NONE),
	  fAutoscrollPulse(NULL),

	  fDisplayRange(NULL),
	  fFirstFrame(-1),
	  fLastFrame(-1),

	  fPlaybackRange(NULL),
	  fFirstPlaybackFrame(-1),
	  fLastPlaybackFrame(-1),
	  fDragStartFrame(0),

	  fLoopMode(NULL),
	  fRangeEnabled(false),

	  fLeftInset(0),
	  fRightInset(0),

	  fFPS(25.0),
	  fDisplayTicks(false),

	  fAnimator(NULL),
	  fItem(NULL),
	  fKeyFrameOffset(0),

	  fKeyPoints(20),
	  fDraggedKey(NULL),
	  fDraggedKeyOffset(0.0),
	  fKeyFrameRemoved(false),

	  fCommandStack(NULL),
	  fKeyFrameCommand(NULL),

	  fTimelineView(NULL)
{
	SetViewColor(B_TRANSPARENT_COLOR);
	BFont font;
	GetFont(&font);
	font.SetSize(9.0);
	SetFont(&font);

	fCurrentFrame->AddObserver(this);
}

// destructor
TimeView::~TimeView()
{
	if (fDisplayRange)
		fDisplayRange->RemoveObserver(this);
	if (fPlaybackRange)
		fPlaybackRange->RemoveObserver(this);
	if (fLoopMode)
		fLoopMode->RemoveObserver(this);

	fCurrentFrame->RemoveObserver(this);

	if (fAnimator)
		fAnimator->RemoveObserver(this);
	if (fItem)
		fItem->RemoveObserver(this);

	delete fKeyFrameCommand;
}

// #pragma mark -

// Draw
void
TimeView::Draw(BRect updateRect)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightShadow = tint_color(background, B_DARKEN_1_TINT);
	rgb_color shadow = tint_color(background, B_DARKEN_2_TINT);
	rgb_color text = tint_color(shadow, B_DARKEN_3_TINT);
	background = tint_color(background, B_LIGHTEN_1_TINT);
	rgb_color darkenMax = tint_color(shadow, B_DARKEN_MAX_TINT);
	rgb_color tickShadow = (rgb_color){ 0, 0, 0, 60 };
	rgb_color tickLight = (rgb_color){ 255, 255, 255, 200 };

	rgb_color playbackRange;
	rgb_color playbackRangeBG;
	if (fRangeEnabled) {
		playbackRange = (rgb_color){ 255, 207, 42, 255 };
		playbackRangeBG = (rgb_color){ 255, 232, 104, 255 };
	} else {
		playbackRange = lightShadow;
		playbackRangeBG = ui_color(B_PANEL_BACKGROUND_COLOR);
	}

	SetLowColor(background);

	BRect r(Bounds());

	// frame
	BeginLineArray(2);
		AddLine(BPoint(r.left, r.top),
				BPoint(r.right, r.top), lightShadow);
		AddLine(BPoint(r.right, r.bottom),
				BPoint(r.left, r.bottom), shadow);
	EndLineArray();

	// playback range
	BRect playbackRect = _PlaybackRangeRect();
	SetHighColor(playbackRangeBG);
	FillRect(playbackRect);

	// background
	r.InsetBy(0.0, 1.0);
	BRegion bg(r);
	bg.Exclude(playbackRect);
	FillRegion(&bg, B_SOLID_LOW);

	font_height fh;
	GetFontHeight(&fh);

	// main markers at full seconds + labels
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

	int64 firstSecond = (int64)floorf(fFirstFrame / fFPS) - 1;
	int64 lastSecond = (int64)ceilf(fLastFrame / fFPS) + 2;

	float labelWidth = StringWidth("00:00:00") + 5.0;
	float distBetweenSeconds = (_PosForFrame(lastSecond * fFPS)
		- _PosForFrame(firstSecond * fFPS)) / (lastSecond - firstSecond + 1);
	int32 ticksPerLabel
		= max_c(1, (int32)ceilf(labelWidth / distBetweenSeconds));
	int32 step = max_c(1, (int32)(8 / distBetweenSeconds));

	// align to make "steps" work
	firstSecond = ((firstSecond - (step - 1)) / step) * step;
	ticksPerLabel = ((ticksPerLabel + (step - 1)) / step) * step;

	SetHighColor(text);

	for (int64 second = firstSecond; second <= lastSecond; second += step) {
		float frame = second * fFPS;
		float tickPos = _PosForFrame(frame);

		float tickHeight = MARK_HEIGHT;

		if (second % ticksPerLabel == 0) {
			BString label = _TimeStringForFrame(frame, false);
			DrawString(label.String(), BPoint(tickPos + 3, r.top + fh.ascent));
			tickHeight = r.bottom - (r.top + fh.ascent) - 1;
		}

		BeginLineArray(2);
			AddLine(BPoint(tickPos, r.bottom),
					BPoint(tickPos, r.bottom - tickHeight), tickShadow);
			AddLine(BPoint(tickPos + 1.0, r.bottom),
					BPoint(tickPos + 1.0, r.bottom - tickHeight), tickLight);
		EndLineArray();
	}

	SetDrawingMode(B_OP_COPY);

	// markers for keyframes
	int32 count = fKeyPoints.CountItems();
	if (count > 0) {
		int32* items = (int32*)fKeyPoints.Items();
		BeginLineArray(count);
		for (int32 i = 0; i < count; i++) {
			AddLine(BPoint(items[i], r.top), BPoint(items[i], r.bottom), kRed);
		}
		EndLineArray();
	}

	// start playback marker
	BRect b = playbackRect;
	b.right = b.left + 5;
	b.top = ceilf((b.top + b.bottom) / 2) - 4;
	b.bottom = b.top + 8;
	if (b.Intersects(updateRect)) {
		// outline
		BeginLineArray(3);
			AddLine(BPoint(b.left, b.bottom),
					BPoint(b.left, b.top), text);
			AddLine(BPoint(b.left + 1, b.top),
					BPoint(b.right, (b.top + b.bottom) / 2), text);
			AddLine(BPoint(b.left + 1, b.bottom),
					BPoint(b.right - 1, (b.top + b.bottom) / 2 + 1), darkenMax);
		EndLineArray();
		// fill
		b.InsetBy(1, 1);
		SetHighColor(playbackRange);
		BPoint triangle[3];
		triangle[0] = BPoint(b.left, b.bottom);
		triangle[1] = BPoint(b.left, b.top);
		triangle[2] = BPoint(b.right, (b.top + b.bottom) / 2);
		FillTriangle(triangle[0], triangle[1], triangle[2]);
		// bevel
		rgb_color playbackRangeLightTop = tint_color(playbackRange, 0.2);
		rgb_color playbackRangeLight
			= tint_color(playbackRange, B_LIGHTEN_2_TINT);
		rgb_color playbackRangeDark
			= tint_color(playbackRange, B_DARKEN_2_TINT);
		BeginLineArray(3);
			AddLine(BPoint(b.left + 1, b.bottom - 1),
					BPoint( b.right - 1, (b.top + b.bottom) / 2 + 1),
					playbackRangeDark);
			AddLine(BPoint(b.left, b.top),
					BPoint(b.right, (b.top + b.bottom) / 2),
					playbackRangeLight);
			AddLine(BPoint(b.left, b.bottom),
					BPoint(b.left, b.top + 1), playbackRangeLightTop);
		EndLineArray();
	}

	// end playback marker
	b = playbackRect;
	b.left = b.right - 5;
	b.top = ceilf((b.top + b.bottom) / 2) - 4;
	b.bottom = b.top + 8;
	if (b.Intersects(updateRect)) {
		// outline
		BeginLineArray(3);
			AddLine(BPoint(b.right, b.bottom),
					BPoint(b.right, b.top), darkenMax);
			AddLine(BPoint(b.right - 1, b.top),
					BPoint(b.left, (b.top + b.bottom) / 2), text);
			AddLine(BPoint(b.right - 1, b.bottom),
					BPoint(b.left + 1, (b.top + b.bottom) / 2 + 1), darkenMax);
		EndLineArray();
		// fill
		b.InsetBy(1, 1);
		SetHighColor(playbackRange);
		BPoint triangle[3];
		triangle[0] = BPoint(b.right, b.bottom);
		triangle[1] = BPoint(b.right, b.top);
		triangle[2] = BPoint(b.left, (b.top + b.bottom) / 2);
		FillTriangle(triangle[0], triangle[1], triangle[2]);
		// bevel
		rgb_color playbackRangeLightTop = tint_color(playbackRange, 0.2);
		rgb_color playbackRangeLight
			= tint_color(playbackRange, B_LIGHTEN_2_TINT);
		rgb_color playbackRangeDark
			= tint_color(playbackRange, B_DARKEN_1_TINT);
		BeginLineArray(3);
			AddLine(BPoint(b.right, b.bottom),
					BPoint(b.right, b.top + 1), playbackRangeDark);
			AddLine(BPoint(b.right, b.top),
					BPoint(b.left, (b.top + b.bottom) / 2),
					playbackRangeLightTop);
			AddLine(BPoint(b.right - 1, b.bottom - 1),
					BPoint( b.left + 1, (b.top + b.bottom) / 2 + 1),
					playbackRangeLight);
		EndLineArray();
	}

	// current frame marker
	r = _MarkerRect();
	if (r.Intersects(updateRect)) {
		// outline
		BeginLineArray(3);
			AddLine(BPoint(r.left, r.top),
					BPoint(r.right, r.top), text);
			AddLine(BPoint(r.right, r.top + 1),
					BPoint((r.left + r.right) / 2, r.bottom), darkenMax);
			AddLine(BPoint(r.left, r.top + 1),
					BPoint((r.left + r.right) / 2 - 1, r.bottom - 1), text);
		EndLineArray();
		// fill
		r.InsetBy(1, 1);
		rgb_color green = (rgb_color){ 0, 255, 0, 255 };
		SetHighColor(green);
		BPoint triangle[3];
		triangle[0] = BPoint(r.left, r.top);
		triangle[1] = BPoint(r.right, r.top);
		triangle[2] = BPoint((r.left + r.right) / 2, r.bottom);
		FillTriangle(triangle[0], triangle[1], triangle[2]);
		// bevel
		rgb_color lightGreenTop = tint_color(green, 0.2);
		rgb_color lightGreen = tint_color(green, B_LIGHTEN_2_TINT);
		rgb_color darkGreen = tint_color(green, B_DARKEN_2_TINT);
		BeginLineArray(3);
			AddLine(BPoint(r.left, r.top),
					BPoint(r.right - 1, r.top), lightGreenTop);
			AddLine(BPoint(r.right, r.top),
					BPoint((r.left + r.right) / 2, r.bottom), darkGreen);
			AddLine(BPoint(r.left + 1, r.top + 1),
					BPoint((r.left + r.right) / 2 - 1, r.bottom - 1), lightGreen);
		EndLineArray();
	}
}

// MessageReceived
void
TimeView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_AUTOSCROLL_PULSE: {
			if (!fAutoscrollPulse)
				break;

			BPoint scrollOffset(0.0, 0.0);
			BRect bounds = Bounds();

			uint32 buttons;
			BPoint mousePos;
			GetMouse(&mousePos, &buttons, false);
			mousePos.ConstrainTo(bounds);

			bounds.InsetBy(40, 0);

			if (!bounds.Contains(mousePos)) {
				// mouse is close to the border
				if (mousePos.x <= bounds.left)
					scrollOffset.x = mousePos.x - bounds.left;
				else if (mousePos.x >= bounds.right)
					scrollOffset.x = mousePos.x - bounds.right;
				if (mousePos.y <= bounds.top)
					scrollOffset.y = mousePos.y - bounds.top;
				else if (mousePos.y >= bounds.bottom)
					scrollOffset.y = mousePos.y - bounds.bottom;

				scrollOffset.x = roundf(scrollOffset.x * 0.8);
				scrollOffset.y = roundf(scrollOffset.y * 0.8);
			}
			if (scrollOffset != B_ORIGIN) {
				BPoint currentScrollOffset = fTimelineView->ScrollOffset();
				fTimelineView->SetScrollOffset(currentScrollOffset
											   + scrollOffset);
				BPoint scrollDiff = fTimelineView->ScrollOffset() - currentScrollOffset;
				if (scrollDiff != B_ORIGIN) {
					GetMouse(&mousePos, &buttons);
					MouseMoved(mousePos, B_INSIDE_VIEW, NULL);
				}
			}

			break;
		}
		default:
			BView::MessageReceived(message);
	}
}

// MouseDown
void
TimeView::MouseDown(BPoint where)
{
	// see if mouse is over a keyframe
	if (fAnimator && !_MarkerRect().Contains(where)) {
		BRect r(Bounds());
		int32 count = fKeyPoints.CountItems();
		for (int32 i = 0; i < count; i++) {
			int32 pos = (int32)(addr_t)fKeyPoints.ItemAtFast(i);
			r.left = pos - 4;
			r.right = pos + 4;
			if (r.Contains(where)) {
				fTracking = TRACKING_KEYFRAME;
				fDraggedKey= fAnimator->KeyFrameAt(i);
				fDraggedKeyOffset = pos - where.x;
				delete fKeyFrameCommand;
				fKeyFrameCommand = new ModifyKeyFrameCommand(fAnimator,
															 fDraggedKey);
				fKeyFrameRemoved = false;
				break;
			}
		}
	}

	// check for direct hit on the marker
	if (fTracking == TRACKING_NONE
		&& _MarkerRect().InsetBySelf(-3, -3).Contains(where)) {
		fTracking = TRACKING_CURRENT_FRAME;
		fDragStartFrame = _FrameForPos(where.x);
		fDragStartFrame = fMarkerPos - fDragStartFrame;
	} else {
		fDragStartFrame = 0;
	}

	if (fTracking == TRACKING_NONE) {
		BRect r = _PlaybackRangeRect();
		r.InsetBy(-3, -3);
		if (r.Contains(where)) {
			fTracking = TRACKING_PLAYBACK_RANGE;
			fDragStartFrame = _FrameForPos(where.x);
			BRect b = r;
			b.right = b.left + 11;
			if (b.Contains(where)) {
				fTracking = TRACKING_PLAYBACK_START;
				fDragStartFrame = fPlaybackRange->FirstFrame() - fDragStartFrame;
			} else {
				b.right = r.right;
				b.left = b.right - 11;
				if (b.Contains(where)) {
					fTracking = TRACKING_PLAYBACK_END;
					fDragStartFrame = fPlaybackRange->LastFrame() - fDragStartFrame;
				}
			}
		}
	}

	// if not dragging any keyframe, we're dragging the current frame
	if (fTracking == TRACKING_CURRENT_FRAME || fTracking == TRACKING_NONE) {
		fTracking = TRACKING_CURRENT_FRAME;
		fCurrentFrame->SetBeingDragged(true);
		_SetMarker(where.x);
	}

	if (fTracking != TRACKING_NONE)
		_SetAutoscrollEnabled(true);

	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

// MouseUp
void
TimeView::MouseUp(BPoint where)
{
	if (fTracking == TRACKING_CURRENT_FRAME)
		fCurrentFrame->SetBeingDragged(false);

	if (fTracking == TRACKING_KEYFRAME && fKeyFrameRemoved) {
		delete fKeyFrameCommand;
		fKeyFrameCommand = new RemoveKeyFrameCommand(fAnimator, fDraggedKey);
		EndRectTracking();
	}

	if (fKeyFrameCommand) {
		if (fCommandStack)
			fCommandStack->Perform(fKeyFrameCommand);
		else
			delete fKeyFrameCommand;
		fKeyFrameCommand = NULL;
	}

	fTracking = TRACKING_NONE;
	_SetAutoscrollEnabled(false);
}

// MouseMoved
void
TimeView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
	switch (fTracking) {
		case TRACKING_CURRENT_FRAME:
			_SetMarker(where.x);
			break;
		case TRACKING_KEYFRAME:
			if (Bounds().Contains(where)) {
				if (fKeyFrameRemoved)
					_AttachKeyFrame();
				_SetKeyFrame(where.x + fDraggedKeyOffset);
			} else
				_DetachKeyFrame(where);
			break;
		case TRACKING_PLAYBACK_START: {
			int64 frame = _FrameForPos(where.x) + fDragStartFrame;
			fLoopMode->SetMode(LOOPING_RANGE);
			if (frame > fPlaybackRange->LastFrame()) {
				fPlaybackRange->SetFrames(fPlaybackRange->LastFrame(), frame);
				fTracking = TRACKING_PLAYBACK_END;
			} else
				fPlaybackRange->SetFirstFrame(frame);
			break;
		}
		case TRACKING_PLAYBACK_END: {
			int64 frame = _FrameForPos(where.x) + fDragStartFrame;
			fLoopMode->SetMode(LOOPING_RANGE);
			if (frame < fPlaybackRange->FirstFrame()) {
				fPlaybackRange->SetFrames(frame, fPlaybackRange->FirstFrame());
				fTracking = TRACKING_PLAYBACK_START;
			} else
				fPlaybackRange->SetLastFrame(frame);
			break;
		}
		case TRACKING_PLAYBACK_RANGE: {
			int64 frame = _FrameForPos(where.x);
			int64 offset = frame - fDragStartFrame;
			fDragStartFrame = frame;
			fLoopMode->SetMode(LOOPING_RANGE);
			fPlaybackRange->MoveBy(offset);
			break;
		}
	}
}

// #pragma mark-

// GetPreferredSize
void
TimeView::GetPreferredSize(float* width, float* height)
{
	if (width != NULL)
		*width = 100.0f;
	if (height != NULL) {
		font_height fh;
		GetFontHeight(&fh);
		*height = 5.0f + fh.ascent + MARK_HEIGHT;
	}
}

// MinSize
BSize
TimeView::MinSize()
{
	BSize size;
	size.width = 10.0f;
	GetPreferredSize(NULL, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}

// MaxSize
BSize
TimeView::MaxSize()
{
	BSize size;
	size.width = B_SIZE_UNLIMITED;
	GetPreferredSize(NULL, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}

// PreferredSize
BSize
TimeView::PreferredSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}

// #pragma mark-

// ObjectChanged
void
TimeView::ObjectChanged(const Observable* object)
{
	if (!LockLooper())
		return;

	if (object == fCurrentFrame) {

		Invalidate(_MarkerRect().InsetBySelf(-1, 0));
		fMarkerPos = fCurrentFrame->VirtualFrame();
		Invalidate(_MarkerRect().InsetBySelf(-1, 0));

	} else if (object == fDisplayRange) {

//		int64 diff = fDisplayRange->FirstFrame() - fFirstFrame;
		int64 oldRange = fLastFrame - fFirstFrame;
		fFirstFrame = fDisplayRange->FirstFrame();
		fLastFrame = fDisplayRange->LastFrame();
//		Invalidate();
		// adjust keyframe offset (rebuild in case of zoom, else simple offset)
		if (oldRange != fLastFrame - fFirstFrame) {
			_RebuildKeyPoints();
			Invalidate();
		}
//		 else if (diff != 0)
//			_MoveKeyFrames(-diff);
		// NOTE: for reasons of hackery, the TimelineView scrolls us along
		// with the display range. The code otherwise respsonsible for that
		// is commented out above

	} else if (object == fAnimator) {

		_RebuildKeyPoints();

	} else if (object == fItem) {

		// adjust keyframe offset
		int64 oldOffset = fKeyFrameOffset;
		fKeyFrameOffset = fItem->StartFrame() - fItem->ClipOffset();
		int64 diff = fKeyFrameOffset - oldOffset;
		if (diff != 0)
			_MoveKeyFrames(diff);

	} else if (object == fPlaybackRange) {

		BRect r = _PlaybackRangeRect();
		fFirstPlaybackFrame = fPlaybackRange->FirstFrame();
		fLastPlaybackFrame = fPlaybackRange->LastFrame();
		r = r | _PlaybackRangeRect();
		Invalidate(r.InsetBySelf(-6, 0));

	} else if (object == fLoopMode) {

		if (fRangeEnabled != (fLoopMode->Mode() == LOOPING_RANGE)) {
			fRangeEnabled = fLoopMode->Mode() == LOOPING_RANGE;
			Invalidate(_PlaybackRangeRect());
		}

	}

	UnlockLooper();
}

// #pragma mark-

// PropertySelected
void
TimeView::PropertyObjectSet(PropertyObject* object)
{
	_SetPlaylistItem(dynamic_cast<PlaylistItem*>(object));
}

// PropertySelected
void
TimeView::PropertySelected(Property* property)
{
	_SetPropertyAnimator(property ? property->Animator() : NULL);
}

// #pragma mark-

// SetDisplayRange
void
TimeView::SetDisplayRange(DisplayRange* range)
{
	if (fDisplayRange != range) {
//		if (fDisplayRange)
//			fDisplayRange->RemoveObserver(this);

		fDisplayRange = range;

//		if (fDisplayRange)
//			fDisplayRange->AddObserver(this);

		// trigger invalidation
		ObjectChanged(fDisplayRange);
	}
}

// FirstFrame
int64
TimeView::FirstFrame() const
{
	return fFirstFrame;
}

// LastFrame
int64
TimeView::LastFrame() const
{
	return fLastFrame;
}

// SetPlaybackRange
void
TimeView::SetPlaybackRange(DisplayRange* range)
{
	if (fPlaybackRange != range) {
		if (fPlaybackRange)
			fPlaybackRange->RemoveObserver(this);

		fPlaybackRange = range;

		if (fPlaybackRange)
			fPlaybackRange->AddObserver(this);

		// trigger invalidation
		ObjectChanged(fPlaybackRange);
	}
}

// SetLoopMode
void
TimeView::SetLoopMode(LoopMode* mode)
{
	if (fLoopMode != mode) {
		if (fLoopMode)
			fLoopMode->RemoveObserver(this);

		fLoopMode = mode;

		if (fLoopMode)
			fLoopMode->AddObserver(this);

		// trigger invalidation
		ObjectChanged(fLoopMode);
	}
}

// SetFramesPerSecond
void
TimeView::SetFramesPerSecond(float fps)
{
	if (fps < 1.0)
		fps = 1.0;
	if (fps != fFPS) {
		fFPS = fps;
		Invalidate();
	}
}

// SetCommandStack
void
TimeView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetInsets
void
TimeView::SetInsets(int32 left, int32 right)
{
	if (left == fLeftInset && right == fRightInset)
		return;

	fLeftInset = left;
	fRightInset = right;

	_RebuildKeyPoints();
	Invalidate();
}

// SetTimelineView
void
TimeView::SetTimelineView(TimelineView* view)
{
	fTimelineView = view;

	_RebuildKeyPoints();
	Invalidate();
}

// #pragma mark -

// _TimeStringForFrame
BString
TimeView::_TimeStringForFrame(int64 frame, bool ticks)
{
	BString timeString("");
	if (fFPS >= 1.0) {
		if (frame < 0) {
			timeString << "-";
			frame = -frame;
		}
		int32 seconds = (int32)(frame / (int32)fFPS);
		int32 minutes = seconds / 60;
		int32 hours = minutes / 60;
		// hours
		if (hours > 0) {
			timeString << hours << ":";
		}
		char string[6];
		// minutes
		sprintf(string,"%0*ld", 2, minutes - hours * 60);
		timeString << string << ":";
		// seconds
		sprintf(string,"%0*ld", 2, seconds - minutes * 60);
		timeString << string;
		// ticks
		if (ticks) {
			timeString << ":";
			sprintf(string,"%0*ld", 5, (int32)(frame % (int64)fFPS) + 1);
			timeString << string;
//			timeString << ":" << (int32)fmod((double)frame, fFPS) + 1;
		}

	}
	return timeString;
}

// _PosForFrame
float
TimeView::_PosForFrame(int64 frame) const
{
	if (fTimelineView)
		return fTimelineView->PosForFrame(frame);

	BRect b(Bounds());
	b.left += fLeftInset;
	b.right -= fRightInset;
	double framesPerPixel;
	if (fTimelineView)
		framesPerPixel = fTimelineView->FramesPerPixel();
	else
		framesPerPixel = (LastFrame() - FirstFrame() + 1) / (b.Width() + 1);
	return b.left + (frame - FirstFrame()) / framesPerPixel;
}

// _FrameForPos
int64
TimeView::_FrameForPos(float pos) const
{
	if (fTimelineView)
		return fTimelineView->FrameForPos(pos);

	BRect b(Bounds());
	b.left += fLeftInset;
	b.right -= fRightInset;
	double framesPerPixel;
	if (fTimelineView)
		framesPerPixel = fTimelineView->FramesPerPixel();
	else
		framesPerPixel = (LastFrame() - FirstFrame() + 1) / (b.Width() + 1);
	return FirstFrame() + (int64)((pos - b.left) * framesPerPixel);
}

// _MarkerRect
BRect
TimeView::_MarkerRect() const
{
	BRect b(Bounds());
	float pos = _PosForFrame(fMarkerPos);
	b.left = pos - 6;
	b.right = pos + 6;
	b.top = b.bottom - 7;
	return b;
}

// _SetMarker
void
TimeView::_SetMarker(float pos)
{
	fCurrentFrame->SetVirtualFrame(_FrameForPos(pos) + fDragStartFrame);
}

// _PlaybackRangeRect
BRect
TimeView::_PlaybackRangeRect() const
{
	BRect b(Bounds());
	b.left = _PosForFrame(fFirstPlaybackFrame);
	b.right = _PosForFrame(fLastPlaybackFrame);
	b.bottom -= 1;
	b.top = b.bottom - 8;
	if (b.left > b.right) {
		float t = b.left;
		b.left = b.right;
		b.right = t;
	}
	return b;
}

// _SetPlaylistItem
void
TimeView::_SetPlaylistItem(PlaylistItem* item)
{
	if (fItem == item)
		return;

	if (fItem)
		fItem->RemoveObserver(this);

	fItem = item;

	if (fItem) {
		fItem->AddObserver(this);
		// remember the local startframe of the item for
		// the offset of keyframes
		fKeyFrameOffset = fItem->StartFrame() - fItem->ClipOffset();
	}

	_RebuildKeyPoints();
}

// _SetPropertyAnimator
void
TimeView::_SetPropertyAnimator(PropertyAnimator* animator)
{
	if (fAnimator == animator)
		return;

	if (fAnimator)
		fAnimator->RemoveObserver(this);

	fAnimator = animator;

	if (fAnimator)
		fAnimator->AddObserver(this);

	_RebuildKeyPoints();
}

// _RebuildKeyPoints
void
TimeView::_RebuildKeyPoints()
{
	BRect r = Bounds();
	r.InsetBy(0.0, 1.0);
	BRegion dirty;

	// invalidate positions of old keyframes
	int32 count = fKeyPoints.CountItems();
	int32* items = (int32*)fKeyPoints.Items();
	for (int32 i = 0; i < count; i++) {
		r.left = r.right = items[i];
		dirty.Include(r);
	}

	// clean the list
	fKeyPoints.MakeEmpty();

	if (!fAnimator) {
		Invalidate(&dirty);
		return;
	}

	// add new keyframes
	count = fAnimator->CountKeyFrames();
	for (int32 i = 0; i < count; i++) {
		KeyFrame* key = fAnimator->KeyFrameAtFast(i);
		// find the horizontal position of this keyframe
		int32 pos = (int32)_PosForFrame(key->Frame() + fKeyFrameOffset);

		if (!fKeyPoints.AddItem((void*)pos))
			break;

		r.left = r.right = pos;
		dirty.Include(r);
	}

	Invalidate(&dirty);
}

// _MoveKeyFrames
void
TimeView::_MoveKeyFrames(int32 offset)
{
	if (offset == 0)
		return;

	BRect r(Bounds());
	r.InsetBy(0.0, 1.0);
	BRegion dirty;

	int32 count = fKeyPoints.CountItems();
	int32* items = (int32*)fKeyPoints.Items();
	for (int32 i = 0; i < count; i++) {
		r.left = r.right = items[i];
		dirty.Include(r);

		items[i] += offset;

		r.left = r.right = items[i];
		dirty.Include(r);
	}

	Invalidate(&dirty);
}

// _SetKeyFrame
void
TimeView::_SetKeyFrame(float pos)
{
	if (!fAnimator || !fDraggedKey)
		return;

	AutoNotificationSuspender _(fAnimator);
	fAnimator->SetKeyFrameFrame(fDraggedKey, _FrameForPos(pos) - fKeyFrameOffset);
}

// _AttachKeyFrame
void
TimeView::_AttachKeyFrame()
{
	if (!fKeyFrameRemoved || !fAnimator || !fDraggedKey)
		return;

	if (!fAnimator->AddKeyFrame(fDraggedKey))
		return;
	fKeyFrameRemoved = false;

	// TODO: hack (trigger property listview to update visually)
	if (fItem)
		fItem->Notify();

	EndRectTracking();
}

// _DetachKeyFrame
void
TimeView::_DetachKeyFrame(BPoint where)
{
	if (fKeyFrameRemoved || !fAnimator || !fDraggedKey)
		return;

	if (!fAnimator->RemoveKeyFrame(fDraggedKey))
		return;
	fKeyFrameRemoved = true;

	// TODO: hack (trigger property listview to update visually)
	if (fItem)
		fItem->Notify();

	BRect r(where, where);
	r.InsetBy(-2, -(Bounds().Height() / 2.0));

	BeginRectTracking(r, B_TRACK_WHOLE_RECT);
}

// _SetAutoscrollEnabled
void
TimeView::_SetAutoscrollEnabled(bool enable)
{
	if (enable && !fAutoscrollPulse) {
		fAutoscrollPulse = new BMessageRunner(BMessenger(this),
			new BMessage(MSG_AUTOSCROLL_PULSE), 40000);
	} else if (!enable && fAutoscrollPulse) {
		delete fAutoscrollPulse;
		fAutoscrollPulse = NULL;
	}
	if (fTimelineView)
		fTimelineView->SetAutoscrollEnabled(!enable);
}




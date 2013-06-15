/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "EditorVideoView.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <Cursor.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Region.h>
#include <Window.h>

#include "cursors.h"
#include "ui_defines.h"
#include "support.h"

#include "MultipleManipulatorState.h"
#include "PlaylistItem.h"
#include "Selection.h"
#include "StageManipulator.h"
#include "StageTool.h"
#include "VideoViewSelection.h"

using std::nothrow;

static const rgb_color kStripeLight = (rgb_color){ 112, 112, 112, 255 };
static const rgb_color kStripeDark = (rgb_color){ 104, 104, 104, 255 };

#define AUTO_SCROLL_DELAY		40000 // 40 ms

enum {
	MSG_AUTO_SCROLL			= 'ascr',
	MSG_DISABLE_OVERLAY		= 'dsao'
};

// constructor
EditorVideoView::EditorVideoView(BRect frame, const char* name)
	: StateView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS)
	, Scrollable()
	, fOverlayMode(false)

	, fPlaylistSelection(NULL)
	, fVideoViewSelection(NULL)
	, fPlaylist(NULL)
	, fTool(NULL)

	, fViewState(new MultipleManipulatorState(this))
	, fCurrentFrame(0)

	, fVideoWidth(684)
	, fVideoHeight(384)

	, fZoomLevel(1.0)
	, fAutoZoomToAll(true)

	, fSpaceHeldDown(false)
	, fScrollTracking(false)
	, fInScrollTo(false)
	, fScrollTrackingStart(0.0, 0.0)
	, fScrollOffsetStart(0.0, 0.0)

	, fAutoScroller(NULL)

	, fOverlayKeyColor(kStripeDark)
{
	SetViewColor(B_TRANSPARENT_32_BIT);
		// will be reset to overlay key color
		// if overlays are used
	SetHighColor(kStripeLight);
	SetLowColor(kStripeDark);
		// used for drawing the stripes pattern
	BFont font(be_bold_font);
	font.SetSize(font.Size() * 1.2);
	SetFont(&font);

	SetState(fViewState);

	// create some hopefully sensible default overlay restrictions
	fOverlayRestrictions.min_width_scale = 0.25;
	fOverlayRestrictions.max_width_scale = 2.0;
	fOverlayRestrictions.min_height_scale = 0.25;
	fOverlayRestrictions.max_height_scale = 2.0;
}

// destructor
EditorVideoView::~EditorVideoView()
{
	SetState(NULL);
	SetSelection(NULL, NULL);
	delete fViewState;
	delete fAutoScroller;
}

// MessageReceived
void
EditorVideoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_DISABLE_OVERLAY:
			DisableOverlay();
			break;

		case MSG_AUTO_SCROLL:
			if (fAutoScroller) {
				BPoint scrollOffset(0.0, 0.0);
				BRect bounds(Bounds());
				BPoint mousePos = MouseInfo()->position;
				mousePos.ConstrainTo(bounds);
				float inset = min_c(min_c(40.0, bounds.Width() / 10),
					min_c(40.0, bounds.Height() / 10));
				bounds.InsetBy(inset, inset);
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
					SetScrollOffset(ScrollOffset() + scrollOffset);
				}
			}
			break;
		default:
			StateView::MessageReceived(message);
			break;
	}
}

// AttachedToWindow
void
EditorVideoView::AttachedToWindow()
{
	StateView::AttachedToWindow();

	// init data rect for scrolling and center bitmap in the view
	BRect dataRect = _LayoutCanvas();
	SetDataRect(dataRect);
	BRect bounds(Bounds());
	BPoint dataRectCenter((dataRect.left + dataRect.right) / 2,
		(dataRect.top + dataRect.bottom) / 2);
	BPoint boundsCenter((bounds.left + bounds.right) / 2,
		(bounds.top + bounds.bottom) / 2);
	SetScrollOffset(ScrollOffset() + BPoint(
			floorf(dataRectCenter.x - boundsCenter.x),
			floorf(dataRectCenter.y - boundsCenter.y)));
}

// FrameResized
void
EditorVideoView::FrameResized(float width, float height)
{
	StateView::FrameResized(width, height);
}

// Draw
void
EditorVideoView::Draw(BRect updateRect)
{
	bool noVideo = true;
	BRect canvas(_CanvasRect());
	if (LockBitmap()) {
		if (const BBitmap* bitmap = GetBitmap()) {
			if (!fOverlayMode) {
				DrawBitmapAsync(bitmap, bitmap->Bounds(), canvas,
					B_FILTER_BITMAP_BILINEAR);
			} else {
				SetHighColor(fOverlayKeyColor);
				FillRect(canvas);
				SetHighColor(kStripeLight);
			}
			noVideo = false;
		}
		UnlockBitmap();
	}

	if (noVideo) {
		DisableOverlay();

		_DrawNoVideoMessage(this, updateRect);
	} else {
		// outside video
		BRegion outside(Bounds() & updateRect);
		outside.Exclude(canvas);
		FillRegion(&outside, kStripes);
	}
	StateView::Draw(updateRect);
}

// #pragma mark -

// MouseDown
void
EditorVideoView::MouseDown(BPoint where)
{
	if (!IsFocus())
		MakeFocus(true);

	uint32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons",
		(int32*)&buttons) < B_OK)
		buttons = 0;

	// handle clicks of the third mouse button ourselves (panning),
	// otherwise have StateView handle it (normal clicks)
	if (fSpaceHeldDown || buttons & B_TERTIARY_MOUSE_BUTTON) {
		// switch into scrolling mode and update cursor
		fScrollTracking = true;
		where.x = roundf(where.x);
		where.y = roundf(where.y);
		fScrollOffsetStart = ScrollOffset();
		fScrollTrackingStart = where - fScrollOffsetStart;
		UpdateStateCursor();
		SetMouseEventMask(B_POINTER_EVENTS,
						  B_LOCK_WINDOW_FOCUS | B_SUSPEND_VIEW_FOCUS);
	} else {
		SetAutoScrolling(true);
		StateView::MouseDown(where);
	}
}

// MouseUp
void
EditorVideoView::MouseUp(BPoint where)
{
	if (fScrollTracking) {
		// stop scroll tracking and update cursor
		fScrollTracking = false;
		UpdateStateCursor();
		// update StateView mouse position
		uint32 transit = Bounds().Contains(where) ?
			B_INSIDE_VIEW : B_OUTSIDE_VIEW;
		StateView::MouseMoved(where, transit, NULL);
	} else {
		StateView::MouseUp(where);
	}
	SetAutoScrolling(false);
}

// MouseMoved
void
EditorVideoView::MouseMoved(BPoint where, uint32 transit, const BMessage* dragMessage)
{
//printf("MouseMoved(BPoint(%.1f, %.1f))\n", where.x, where.y);
	if (fScrollTracking) {
		uint32 buttons;
		GetMouse(&where, &buttons, false);
		if (!buttons) {
			MouseUp(where);
			return;
		}
		where.x = roundf(where.x);
		where.y = roundf(where.y);
		where -= ScrollOffset();
		BPoint offset = where - fScrollTrackingStart;
		SetScrollOffset(fScrollOffsetStart - offset);
		UpdateStateCursor();
	} else {
		// normal mouse movement handled by StateView
		if (!fSpaceHeldDown)
			StateView::MouseMoved(where, transit, dragMessage);
		else
			UpdateStateCursor();
	}
}

// #pragma mark -

// MouseWheelChanged
bool
EditorVideoView::MouseWheelChanged(BPoint where, float x, float y)
{
	if (!Bounds().Contains(where))
		return false;

	if (y > 0.0) {
		SetZoomLevel(NextZoomOutLevel(fZoomLevel), true);
		fAutoZoomToAll = false;
		return true;
	} else if (y < 0.0) {
		SetZoomLevel(NextZoomInLevel(fZoomLevel), true);
		fAutoZoomToAll = false;
		return true;
	}
	return false;
}

// UpdateStateCursor
void
EditorVideoView::UpdateStateCursor()
{
	if (fScrollTracking || fSpaceHeldDown) {
		// indicate scrolling mode
		const uchar* cursorData = fScrollTracking ? kGrabCursor : kHandCursor;
		BCursor cursor(cursorData);
		SetViewCursor(&cursor, true);
	} else {
		// pass on to current state of StateView
		StateView::UpdateStateCursor();
	}
}

// ViewStateBoundsChanged
void
EditorVideoView::ViewStateBoundsChanged()
{
	if (!Window())
		return;
//	if (fScrollTracking)
//		return;

//	fScrollTracking = true;
	SetDataRect(_LayoutCanvas());
//	fScrollTracking = false;
}

// #pragma mark -

// ConvertFromCanvas
void
EditorVideoView::ConvertFromCanvas(BPoint* point) const
{
	point->x *= fZoomLevel;
	point->y *= fZoomLevel;
}

// ConvertToCanvas
void
EditorVideoView::ConvertToCanvas(BPoint* point) const
{
	point->x /= fZoomLevel;
	point->y /= fZoomLevel;
}

// ConvertFromCanvas
void
EditorVideoView::ConvertFromCanvas(BRect* r) const
{
	r->left *= fZoomLevel;
	r->top *= fZoomLevel;
	r->right++;
	r->bottom++;
	r->right *= fZoomLevel;
	r->bottom *= fZoomLevel;
	r->right--;
	r->bottom--;
}

// ConvertToCanvas
void
EditorVideoView::ConvertToCanvas(BRect* r) const
{
	r->left /= fZoomLevel;
	r->right /= fZoomLevel;
	r->top /= fZoomLevel;
	r->bottom /= fZoomLevel;
}

// #pragma mark -

// SetScrollOffset
void
EditorVideoView::SetScrollOffset(BPoint newOffset)
{
	if (fInScrollTo)
		return;

	fInScrollTo = true;

	newOffset = ValidScrollOffsetFor(newOffset);
	if (!fScrollTracking) {
		BPoint mouseOffset = newOffset - ScrollOffset();
		MouseMoved(fMouseInfo.position + mouseOffset, fMouseInfo.transit, NULL);
	}

	Scrollable::SetScrollOffset(newOffset);

	fInScrollTo = false;
}

// ScrollOffsetChanged
void
EditorVideoView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	BPoint offset = newOffset - oldOffset;
//printf("EditorVideoView::ScrollOffsetChanged(%.1f, %.1f)\n", offset.x, offset.y);

	if (offset == B_ORIGIN)
		// prevent circular code (MouseMoved might call ScrollBy...)
		return;

	ScrollBy(offset.x, offset.y);
	if (fOverlayMode && LockBitmap()) {
		if (const BBitmap* bitmap = GetBitmap()) {
			rgb_color key;
			SetViewOverlay(bitmap,
						   bitmap->Bounds(),
						   _CanvasRect(),
						   &key, B_FOLLOW_NONE,
						   _OverlayFilterFlags()
						   | B_OVERLAY_TRANSFER_CHANNEL);
		}
		UnlockBitmap();
	}
}

// VisibleSizeChanged
void
EditorVideoView::VisibleSizeChanged(float oldWidth, float oldHeight,
	float newWidth, float newHeight)
{
	if (fAutoZoomToAll)
		_ZoomToAll(newWidth, newHeight);
	SetDataRect(_LayoutCanvas());
}

// #pragma mark -

// SetBitmap
void
EditorVideoView::SetBitmap(const BBitmap* bitmap)
{
	VCTarget::SetBitmap(bitmap);
	// Attention: Don't lock the window, if the bitmap is NULL. Otherwise
	// we're going to deadlock when the window tells the node manager to
	// stop the nodes (Window -> NodeManager -> VideoConsumer
	// -> EditorVideoView -> Window).
	if (bitmap && LockLooperWithTimeout(10000) == B_OK) {
		if (LockBitmap()) {
//			if (fOverlayMode || bitmap->Flags() & B_BITMAP_WILL_OVERLAY) {
			if (fOverlayMode || bitmap->ColorSpace() == B_YCbCr422) {
				if (!fOverlayMode) {
					// init overlay
					rgb_color key;
					status_t ret = SetViewOverlay(bitmap, bitmap->Bounds(),
						_CanvasRect(), &key, B_FOLLOW_NONE,
						_OverlayFilterFlags());
					if (ret == B_OK) {
						fOverlayKeyColor = key;
						Invalidate(_CanvasRect());
						// use overlay from here on
						fOverlayMode = true;

						// update restrictions
						overlay_restrictions restrictions;
						if (bitmap->GetOverlayRestrictions(&restrictions) == B_OK)
							fOverlayRestrictions = restrictions;
					} else {
						// try again next time
						_SynchronousDraw();
					}
				} else {
					// transfer overlay channel
					rgb_color key;
					SetViewOverlay(bitmap,
								   bitmap->Bounds(),
								   _CanvasRect(),
								   &key, B_FOLLOW_NONE,
								   _OverlayFilterFlags()
								   | B_OVERLAY_TRANSFER_CHANNEL);
				}
			} else if (fOverlayMode && bitmap->ColorSpace() != B_YCbCr422) {
				fOverlayMode = false;
				ClearViewOverlay();
			}
			if (!fOverlayMode) {
				// draw the bitmap and the manipulators synchronously
				_SynchronousDraw();
			}
			UnlockBitmap();
		}
		UnlockLooper();
	} else if (!bitmap) {
		Looper()->PostMessage(MSG_DISABLE_OVERLAY, this);
	}
}

// #pragma mark -

// ObjectChanged
void
EditorVideoView::ObjectChanged(const Observable* object)
{
	if (object == fPlaylistSelection) {
		_RebuildManipulators();
		UpdateStateCursor();
	}
}

// #pragma mark -

// NextZoomInLevel
double
EditorVideoView::NextZoomInLevel(double zoom) const
{
	if (zoom < 0.25)
		return 0.25;
	if (zoom < 0.33)
		return 0.33;
	if (zoom < 0.5)
		return 0.5;
	if (zoom < 0.66)
		return 0.66;
	if (zoom < 1)
		return 1;
	if (zoom < 1.5)
		return 1.5;
	if (zoom < 2)
		return 2;
	if (zoom < 3)
		return 3;
	if (zoom < 4)
		return 4;
	if (zoom < 6)
		return 6;
	if (zoom < 8)
		return 8;
	if (zoom < 16)
		return 16;
	if (zoom < 32)
		return 32;
	return 64;
}

// NextZoomOutLevel
double
EditorVideoView::NextZoomOutLevel(double zoom) const
{
	if (zoom > 32)
		return 32;
	if (zoom > 16)
		return 16;
	if (zoom > 8)
		return 8;
	if (zoom > 6)
		return 6;
	if (zoom > 4)
		return 4;
	if (zoom > 3)
		return 3;
	if (zoom > 2)
		return 2;
	if (zoom > 1.5)
		return 1.5;
	if (zoom > 1.0)
		return 1.0;
	if (zoom > 0.66)
		return 0.66;
	if (zoom > 0.5)
		return 0.5;
	if (zoom > 0.33)
		return 0.33;
	return 0.25;
}

// SetZoomLevel
void
EditorVideoView::SetZoomLevel(double zoomLevel, bool mouseIsAnchor)
{
	if (fZoomLevel == zoomLevel)
		return;

	BPoint anchor;
	if (mouseIsAnchor) {
		// zoom into mouse position
		anchor = MouseInfo()->position;
	} else {
		// zoom into center of view
		BRect bounds(Bounds());
		anchor.x = (bounds.left + bounds.right + 1) / 2.0;
		anchor.y = (bounds.top + bounds.bottom + 1) / 2.0;
	}

	BPoint canvasAnchor = anchor;
	ConvertToCanvas(&canvasAnchor);

	fZoomLevel = zoomLevel;
	BRect dataRect = _LayoutCanvas();

	ConvertFromCanvas(&canvasAnchor);

	BPoint offset = ScrollOffset();
	offset.x = roundf(offset.x + canvasAnchor.x - anchor.x);
	offset.y = roundf(offset.y + canvasAnchor.y - anchor.y);

	Invalidate();

	SetDataRectAndScrollOffset(dataRect, offset);
}

// SetAutoZoomToAll
void
EditorVideoView::SetAutoZoomToAll(bool autoZoom)
{
	fAutoZoomToAll = autoZoom;
	if (fAutoZoomToAll) {
		BRect bounds = VisibleBounds();
		_ZoomToAll(bounds.Width(), bounds.Height());
	}
}

// ZoomToAll
void
EditorVideoView::_ZoomToAll(float width, float height)
{
	float minZoomX = width / (fVideoWidth - 1);
	float minZoomY = height / (fVideoHeight - 1);
	float zoom = min_c(minZoomX, minZoomY);
	if (int(zoom * 10) == 10)
		zoom = 1.0;
	SetZoomLevel(zoom);
}

// SetAutoScrolling
void
EditorVideoView::SetAutoScrolling(bool scroll)
{
	if (scroll) {
		if (!fAutoScroller) {
			BMessenger messenger(this, Window());
			BMessage message(MSG_AUTO_SCROLL);
			// this trick avoids the MouseMoved() hook
			// to think that the mouse is not pressed
			// anymore when we call it ourselfs from the
			// autoscrolling code
			message.AddInt32("buttons", 1);
			fAutoScroller = new BMessageRunner(messenger,
											   &message,
											   AUTO_SCROLL_DELAY);
		}
	} else {
		delete fAutoScroller;
		fAutoScroller = NULL;
	}
}

// #pragma mark -

// SetVideoSize
void
EditorVideoView::SetVideoSize(uint32 width, uint32 height)
{
	fVideoWidth = width;
	fVideoHeight = height;
	FrameResized(Bounds().Width(), Bounds().Height());
}

// DisableOverlay
void
EditorVideoView::DisableOverlay()
{
	if (fOverlayMode) {
		ClearViewOverlay();
		snooze(40000);
		Sync();
		fOverlayMode = false;
		Draw(Bounds());
		Sync();
	}
}

// SetTool
void
EditorVideoView::SetTool(StageTool* tool)
{
	if (fTool == tool)
		return;

	fTool = tool;

	_RebuildManipulators();
	UpdateStateCursor();
}

// SetSelection
void
EditorVideoView::SetSelection(Selection* playlistSelection,
	VideoViewSelection* videoViewSelection)
{
	if (fPlaylistSelection != playlistSelection) {
		if (fPlaylistSelection)
			fPlaylistSelection->RemoveObserver(this);

		fPlaylistSelection = playlistSelection;

		if (fPlaylistSelection)
			fPlaylistSelection->AddObserver(this);

		_RebuildManipulators();
		UpdateStateCursor();
	}

	fVideoViewSelection = videoViewSelection;
}

// SetPlaylist
void
EditorVideoView::SetPlaylist(::Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	fPlaylist = playlist;

	_RebuildManipulators();
	UpdateStateCursor();
}

// SetCurrentFrame
void
EditorVideoView::SetCurrentFrame(int64 frame)
{
	if (fCurrentFrame == frame)
		return;

	fCurrentFrame = frame;

	_UpdateManipulatorsWithCurrentFrame();
}

// GetOverlayScaleLimits
void
EditorVideoView::GetOverlayScaleLimits(float* minScale, float* maxScale) const
{
	*minScale = max_c(fOverlayRestrictions.min_width_scale,
		fOverlayRestrictions.min_height_scale);
	*maxScale = max_c(fOverlayRestrictions.max_width_scale,
		fOverlayRestrictions.max_height_scale);
}

// HandlesAllKeyEvents
bool
EditorVideoView::HandlesAllKeyEvents() const
{
	return fViewState && fViewState->HandlesAllKeyEvents();
}

// #pragma mark -

// _HandleKeyDown
bool
EditorVideoView::_HandleKeyDown(const StateView::KeyEvent& event,
	BHandler* originalHandler)
{
	if (HandlesAllKeyEvents()) {
		// the current state handles the key events
		return false;
	}

	switch (event.key) {
		case 'z':
		case 'y':
//			if (modifiers & B_SHIFT_KEY)
//				CommandStack()->Redo();
//			else
//				CommandStack()->Undo();
			break;

		case '+':
			SetZoomLevel(NextZoomInLevel(fZoomLevel));
			break;
		case '-':
			SetZoomLevel(NextZoomOutLevel(fZoomLevel));
			break;

		case B_SPACE:
			fSpaceHeldDown = true;
			UpdateStateCursor();
			break;

		default:
			return StateView::_HandleKeyDown(event, originalHandler);
	}

	return true;
}

// _HandleKeyUp
bool
EditorVideoView::_HandleKeyUp(const StateView::KeyEvent& event,
	BHandler* originalHandler)
{
	switch (event.key) {
		case B_SPACE:
			fSpaceHeldDown = false;
			UpdateStateCursor();
			break;

		default:
			return StateView::_HandleKeyUp(event, originalHandler);
	}

	return true;
}

// #pragma mark -

// _CanvasRect()
BRect
EditorVideoView::_CanvasRect() const
{
	BRect r(0, 0, fVideoWidth - 1, fVideoHeight - 1);
	ConvertFromCanvas(&r);
	r.right = floorf(r.right + 0.5);
	r.bottom = floorf(r.bottom + 0.5);
	return r;
}

// _LayoutCanvas
BRect
EditorVideoView::_LayoutCanvas()
{
	// size of zoomed bitmap
	BRect r(_CanvasRect());
	r.OffsetTo(B_ORIGIN);

	// ask current view state to extend size
	BRect stateBounds = ViewStateBounds();

	// resize for empty area around bitmap
	// (the size we want, but might still be much smaller than view)
	if (stateBounds.IsValid())
		r.InsetBy(-50, -50);

	// center data rect in bounds
	BRect bounds(Bounds());
	if (bounds.Width() > r.Width()) {
		float tooMuch = bounds.Width() - r.Width();
		float half = floorf(tooMuch / 2);
		r.left -= half;
		r.right += tooMuch - half;
	}
	if (bounds.Height() > r.Height()) {
		float tooMuch = bounds.Height() - r.Height();
		float half = floorf(tooMuch / 2);
		r.top -= half;
		r.bottom += tooMuch - half;
	}

	if (stateBounds.IsValid()) {
		stateBounds.InsetBy(-20, -20);
		r = r | stateBounds;
	}

	return r;
}

// #pragma mark -

// _RebuildManipulators
void
EditorVideoView::_RebuildManipulators()
{
	// TODO: rebuilding manipulators should be "up to the current tool"
	if (!fPlaylistSelection || !fTool)
		return;

	fViewState->DeleteManipulators();
	fVideoViewSelection->SetAssociatedSelectable(NULL);

	int32 count = fPlaylistSelection->CountSelected();
	PlaylistItem** objects = NULL;
	if (count > 0) {
		objects = new (nothrow) PlaylistItem*[count];
		if (objects == NULL)
			return;
	}
	bool foundPlaylistItem = false;
	for (int32 i = 0; i < count; i++) {
		objects[i] = dynamic_cast<PlaylistItem*>(
			fPlaylistSelection->SelectableAt(i));
		foundPlaylistItem = objects[i] || foundPlaylistItem;
	}

//printf("invoking AddManipulators(this=%p, fViewState=%p, objects=%p, "
//"count=%ld, fPlaylist=%p, fVideoViewSelection=%p)\n",
//this, fViewState, objects, count, fPlaylist, fVideoViewSelection);
	if (fTool->AddManipulators(this, fViewState, objects, count,
			fPlaylist, fVideoViewSelection)) {
		_UpdateManipulatorsWithCurrentFrame();
	}

	delete[] objects;
}

// _UpdateManipulatorsWithCurrentFrame
void
EditorVideoView::_UpdateManipulatorsWithCurrentFrame()
{
	int32 count = fViewState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		StageManipulator* manipulator
			= dynamic_cast<StageManipulator*>(
				fViewState->ManipulatorAtFast(i));
		if (manipulator)
			manipulator->SetCurrentFrame(fCurrentFrame);
	}
}

// _SynchronousDraw
void
EditorVideoView::_SynchronousDraw()
{
//	PushState();
//	Window()->BeginViewTransaction();
//	Draw(Bounds());
//	Window()->EndViewTransaction();
//	PopState();
//	Sync();
	Invalidate();
}

// _DrawNoVideoMessage
void
EditorVideoView::_DrawNoVideoMessage(BView* into, BRect updateRect)
{
	into->PushState();

	const char* message = "Preparing video display...";
	font_height fh;
	into->GetFontHeight(&fh);
	float width = into->StringWidth(message);
	float height = ceilf(fh.ascent + fh.descent);
	BRect center(into->Bounds());
	center.left = floorf((center.left + center.right - width * 1.3) / 2.0);
	center.right = ceilf(center.left + width * 1.3);
	center.top = floorf((center.top + center.bottom - height * 2.0) / 2.0);
	center.bottom = ceilf(center.top + height * 2.0);
	BRegion stripesArea(updateRect);
	stripesArea.Exclude(center);
	into->FillRegion(&stripesArea, kStripes);
	if (updateRect.Intersects(center)) {
		into->FillRect(center, B_SOLID_LOW);
		BPoint pos;
		pos.x = (center.left + center.right - width) / 2.0; 
		pos.y = (center.top + center.bottom + fh.ascent) / 2.0;
		into->SetHighColor(tint_color(into->HighColor(), 0.8));
		into->DrawString(message, pos);
	}

	into->PopState();
}

// _OverlayFilterFlags
uint32
EditorVideoView::_OverlayFilterFlags()
{
	if (fZoomLevel < 1.0)
		return B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL;
	return 0;
}


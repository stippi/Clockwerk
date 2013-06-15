/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PropertyManipulator.h"

#include <stdio.h>

#include <Region.h>

#include "support.h"

#include "AddKeyFrameCommand.h"
#include "KeyFrame.h"
#include "ModifyKeyFrameCommand.h"
#include "PlaylistItem.h"
#include "SplitManipulator.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "TimelineView.h"

enum {
	TRACKING_NONE = 0,

	TRACKING_PICK_KEY,
	TRACKING_ADD_KEY,
	TRACKING_DRAG_KEY,
};

// constructor
PropertyManipulator::PropertyManipulator(SplitManipulator* parent,
										 PropertyAnimator* animator)
	: Manipulator(animator),
	  fParent(parent),

	  fAnimator(animator),

	  fKeyPoints(20),
	  fCachedClipOffset(0),
	  fCachedDuration(fParent->_Item()->Duration()),

	  fTracking(TRACKING_NONE),
	  fStartFrame(-1),
	  fFrameFixed(false),
	  fKeyIndex(-1),
	  fDraggedKey(NULL),
	  fCommand(NULL)
{
	fParent->_Item()->AddObserver(this);
}

// destructor
PropertyManipulator::~PropertyManipulator()
{
	fParent->_Item()->RemoveObserver(this);
	delete fCommand;
}

// ObjectChanged
void
PropertyManipulator::ObjectChanged(const Observable* object)
{
	bool rebuild = false;

	if (object == fAnimator) {
		rebuild = true;
	} else if (object == fParent->_Item()) {
		if (fParent->_Item()->ClipOffset() != fCachedClipOffset
			|| fParent->_Item()->Duration() != fCachedDuration) {
			rebuild = true;
			fCachedClipOffset = fParent->_Item()->ClipOffset();
			fCachedDuration = fParent->_Item()->Duration();
		}
	}

	if (rebuild)
		RebuildCachedData();
}

// #pragma mark -

// Draw
void
PropertyManipulator::Draw(BView* into, BRect updateRect)
{
	BRect frame = fParent->_ItemFrame();
	frame.InsetBy(0.0, 2.0);
	frame.right--;
	frame.bottom++;

	if (!frame.IsValid())
		return;

	into->PushState();
	BRegion region(frame);
	into->ConstrainClippingRegion(&region);
	into->SetDrawingMode(B_OP_ALPHA);
	into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

	if (fKeyPoints.IsEmpty())
		_BuildPropertyPath();

	// connect the key with a line
	int32 count = fKeyPoints.CountItems();
	BPoint previousKeyPos;
	for (int32 i = 0; i < count; i++) {
		// get the keyframe information
		BPoint keyPos(*fKeyPoints.ItemAtFast(i));

		keyPos.x += frame.left;
		keyPos.y += frame.top;

		if (i > 0) {
			// draw a line connecting the previous
			// and this key frame
			into->SetHighColor(0, 0, 0, 85);
			into->StrokeLine(previousKeyPos, keyPos);
		}

		previousKeyPos = keyPos;
	}

	// stroke rects at the keys
	for (int32 i = 0; i < count; i++) {
		// get the keyframe information
		BPoint keyPos(*fKeyPoints.ItemAtFast(i));

		keyPos.x += frame.left;
		keyPos.y += frame.top;

		// stroke a rect for the key
		into->SetHighColor(0, 0, 0, 165);
		if (keyPos.x < frame.left) {
			rgb_color c = into->HighColor();
			into->BeginLineArray(3);
				into->AddLine(BPoint(keyPos.x, keyPos.y - 1),
							  BPoint(keyPos.x + 1, keyPos.y - 1), c);
				into->AddLine(BPoint(keyPos.x + 1, keyPos.y),
							  BPoint(keyPos.x + 1, keyPos.y), c);
				into->AddLine(BPoint(keyPos.x, keyPos.y + 1),
							  BPoint(keyPos.x + 1, keyPos.y + 1), c);
			into->EndLineArray();
		} else {
			into->StrokeRect(BRect(keyPos, keyPos).InsetBySelf(-1.0, -1.0));
		}
		into->SetHighColor(255, 255, 255, 240);
		into->StrokeLine(keyPos, keyPos);
	}

	into->PopState();
}

// MouseDown
bool
PropertyManipulator::MouseDown(BPoint where)
{
	if (fTracking == TRACKING_NONE)
		return false;

	fDraggedKey = NULL;
	delete fCommand;
	fCommand = NULL;

	if (fTracking == TRACKING_PICK_KEY) {
		fDraggedKey = fAnimator->KeyFrameAt(fKeyIndex);
		if (fDraggedKey)
			fCommand = new ModifyKeyFrameCommand(fAnimator, fDraggedKey);
		else
			fTracking = TRACKING_ADD_KEY;
	}

	if (fTracking == TRACKING_ADD_KEY) {
		// get frame at mouse pos
		int64 frame = fParent->_View()->FrameForPos(where.x);
		// convert to clip local frame
		frame -= fParent->_Item()->StartFrame()
					- fParent->_Item()->ClipOffset();
		// insert key frame and remember it
		fDraggedKey = fAnimator->InsertKeyFrameAt(frame);
		fCommand = new AddKeyFrameCommand(fAnimator, fDraggedKey);
	}

	if (fDraggedKey) {
		_SetTracking(TRACKING_DRAG_KEY);

		int64 itemStartFrame = fParent->_Item()->StartFrame();
		int64 itemEndFrame = fParent->_Item()->EndFrame();
		uint64 clipOffset = fParent->_Item()->ClipOffset();
		// remember frame from dragged keyframe
		fStartFrame = fDraggedKey->Frame() + (itemStartFrame - clipOffset);
		// see if the frame is fixed
		fFrameFixed = fStartFrame == itemStartFrame
						|| fStartFrame == itemEndFrame;
		return true;
	}

	return false;
}

// MouseMoved
void
PropertyManipulator::MouseMoved(BPoint where)
{
	if (fDraggedKey) {
		int64 itemStartFrame = fParent->_Item()->StartFrame();
		int64 itemEndFrame = fParent->_Item()->EndFrame();
		uint64 clipOffset = fParent->_Item()->ClipOffset();

		// get frame at mouse pos
		int64 frame = fParent->_View()->FrameForPos(where.x);

		// filter frame
		if (fFrameFixed)
			// prevent changes to frame if it is "fixed"
			frame = fStartFrame;
		else {
			// truncate frame to item range
			if (frame < itemStartFrame)
				frame = itemStartFrame;
			else if (frame > itemEndFrame)
				frame = itemEndFrame;
		}

		// convert to item local frame
		frame -= itemStartFrame;
		// compensate item clip offset
		frame += clipOffset;

		// adjust property
		Property* property = fDraggedKey->Property();
		if (!property) {
			// ups...
			return;
		}
		BRect r = fParent->_ItemFrame();
		r.InsetBy(0.0, 2.0);
		float top = (r.top + r.bottom) / 2.0;
		float bottom = r.bottom;

		// find the property value for the mouse position
		float scale = (bottom - where.y) / (bottom - top);
//printf("scale: %.2f -> value: %.2f\n", scale, value);

		fAnimator->SuspendNotifications(true);

		// set the property value
		property->SetScale(scale);

		// move keyframe location
		// NOTE: do this after the property value has changed,
		// since SetKeyFrameFrame() will trigger a notification
		fAnimator->SetKeyFrameFrame(fDraggedKey, frame);

		// make sure a notifaction is triggered
		fAnimator->Notify();
		fAnimator->SuspendNotifications(false);
	}
}

// MouseUp
Command*
PropertyManipulator::MouseUp()
{
	fDraggedKey = NULL;
	fTracking = TRACKING_NONE;
	fKeyIndex = -1;

	Command* command = fCommand;
	fCommand = NULL;
	return command;
}

#define HIT_TEST_DIST	7.0

// MouseOver
bool
PropertyManipulator::MouseOver(BPoint where)
{
	// convert "where" into local clip coordinate space
	BRect frame = fParent->_ItemFrame();
	frame.top += 2.0;
	where.x -= frame.left;
	where.y -= frame.top;

	// initial reaction
	uint32 tracking = TRACKING_NONE;
	int32 keyIndex = -1;

	// check if cursor is above keyframe
	float shortestDist = HIT_TEST_DIST + 1;
	int32 count = fKeyPoints.CountItems();
	for (int32 i = 0; i < count; i++) {
		BPoint* p = fKeyPoints.ItemAtFast(i);
		float dist = point_point_distance(where, *p);
		if (dist < shortestDist) {
			tracking = TRACKING_PICK_KEY;
			keyIndex = i;
			shortestDist = dist;
		}
	}

	if (tracking == TRACKING_NONE) {
		// check if cursor is on line between key frames
		for (int32 i = 1; i < count; i++) {
			BPoint* a = fKeyPoints.ItemAtFast(i - 1);
			BPoint* b = fKeyPoints.ItemAtFast(i);
			double dist = point_line_distance(where, *a, *b);
			if (dist <= HIT_TEST_DIST) {
				tracking = TRACKING_ADD_KEY;
				keyIndex = i - 1;
				break;
			}
		}
	}

	if (fTracking != tracking || fKeyIndex != keyIndex) {
		fTracking = tracking;
		fKeyIndex = keyIndex;
		// TODO: handle invalidation and stuff
		// (hit indication, changing cursor...)
	}

	return fTracking > TRACKING_NONE;
}

// Bounds
BRect
PropertyManipulator::Bounds()
{
	return fParent->_ItemFrame();
}

// RebuildCachedData
void
PropertyManipulator::RebuildCachedData()
{
	fKeyPoints.MakeEmpty();
	if (fParent->_View() && fParent->_View()->LockLooper()) {
		fParent->_View()->Invalidate(fParent->_ItemFrame());
		fParent->_View()->UnlockLooper();
	}
}

// #pragma mark -

// _SetTracking
void
PropertyManipulator::_SetTracking(uint32 mode)
{
	if (fTracking != mode) {
		fTracking = mode;
	}
}

// _BuildPropertyPath
void
PropertyManipulator::_BuildPropertyPath()
{
	if (!fParent->_View())
		return;

	BRect frame = fParent->_ItemFrame();
	frame.InsetBy(0.0, 2.0);
	frame.right--;

	if (!frame.IsValid())
		return;

	// offset for clip local start frame
	int64 offset = fParent->_Item()->StartFrame() - fParent->_Item()->ClipOffset();

	// top coordinate for "max" property value
	float top = roundf((frame.top + frame.bottom) / 2.0);
	// bottom coordinate for "min" property value
	float bottom = frame.bottom;

	fKeyPoints.MakeEmpty();

	if (!fAnimator)
		return;

	bool insertFakeFirst = true;
	bool insertFakeLast = true;

	int32 count = fAnimator->CountKeyFrames();
	for (int32 i = 0; i < count; i++) {
		// get the keyframe information
		KeyFrame* key = fAnimator->KeyFrameAtFast(i);

		// the vertical position of the keyframe
		// is the propertys scale, if the property
		// has no "range", it defaults to the middle
		// of the area
		float scale = key->Scale();

		// find the position of this keyframe and value
		BPoint* keyPos = new BPoint();
		keyPos->x = fParent->_View()->PosForFrame(key->Frame() + offset);
		keyPos->y = roundf(bottom - (bottom - top) * scale);

		// map to rect at origin;
		keyPos->x -= frame.left;
		keyPos->y -= frame.top;

		if (keyPos->x == 0)
			insertFakeFirst = false;
		if (keyPos->x == frame.Width())
			insertFakeLast = false;

		fKeyPoints.AddItem(keyPos);
	}

	if (insertFakeFirst) {
		float scale = fAnimator->ScaleAt(fParent->_Item()->ClipOffset());

		// find the position of this keyframe and value
		BPoint* keyPos = new BPoint();
		keyPos->x = 0;
		keyPos->y = roundf(bottom - (bottom - top) * scale);

		// map to rect at origin;
		keyPos->y -= frame.top;

		// insert fake key at right index
		int32 index = 0;
		count = fKeyPoints.CountItems();
		for (int32 i = 0; i < count; i++) {
			BPoint* p = (BPoint*)fKeyPoints.ItemAtFast(i);
			if (p->x < keyPos->x)
				index++;
		}
		fKeyPoints.AddItem(keyPos, index);
	}

	if (insertFakeLast) {
		float scale = fAnimator->ScaleAt(fParent->_Item()->Duration() - 1);

		// find the position of this keyframe and value
		BPoint* keyPos = new BPoint();
		keyPos->x = frame.Width();
		keyPos->y = roundf(bottom - (bottom - top) * scale);

		// map to rect at origin;
		keyPos->y -= frame.top;

		// insert fake key at right index
		int32 index = 0;
		count = fKeyPoints.CountItems();
		for (int32 i = 0; i < count; i++) {
			BPoint* p = (BPoint*)fKeyPoints.ItemAtFast(i);
			if (p->x < keyPos->x)
				index++;
		}
		fKeyPoints.AddItem(keyPos, index);
	}
}

// _DirtyRectForKey
BRect
PropertyManipulator::_DirtyRectForKey(KeyFrame* key) const
{
	return _DirtyRectForKey(fAnimator->IndexOf(key));
}

// _DirtyRectForKey
BRect
PropertyManipulator::_DirtyRectForKey(int32 keyIndex) const
{
	BPoint first(0, 0);
	BPoint second(-1, -1);

	if (BPoint* p = fKeyPoints.ItemAt(keyIndex)) {
		// init points
		first.x = p->x - 1;
		first.y = p->y - 1;
		second.x = p->x + 1;
		second.y = p->y + 1;
	} else {
		// index out of range
		return BRect(0, 0, -1, -1);
	}

	// NOTE: since we will almost certainly have to
	// redraw the line between points, we search for
	// the keys before and after the given index and
	// return the whole area so that gets invalidated

	if (BPoint* p = fKeyPoints.ItemAt(keyIndex - 1)) {
		// there is a key before the asked index
		first.x = p->x - 1;
		first.y = p->y - 1;
	}

	if (BPoint* p = fKeyPoints.ItemAt(keyIndex + 1)) {
		// there is a key before the asked index
		second.x = p->x + 1;
		second.y = p->y + 1;
	}

	return BRect(first.x, min_c(first.y, second.y),
				 second.x, max_c(first.y, second.y));
}


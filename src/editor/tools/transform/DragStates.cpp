/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <math.h>
#include <stdio.h>

#include <Cursor.h>
#include <InterfaceDefs.h>
#include <View.h>

#include "cursors.h"
#include "support.h"

#include "TransformBox.h"
//#include "Strings.h"

#include "DragStates.h"

static double
snap_scale(double scale)
{
	double sign = scale < 0.0 ? -1.0 : 1.0;
	scale = fabs(scale);

	if (fabs(0.125 - scale) < 0.125 * 0.1)
		return 0.125 * sign;
	if (fabs(0.25 - scale) < 0.25 * 0.1)
		return 0.25 * sign;
	if (fabs(0.5 - scale) < 0.5 * 0.1)
		return 0.5 * sign;
	if (fabs(1 - scale) < 1 * 0.1)
		return 1 * sign;
	if (fabs(1.5 - scale) < 1.5 * 0.1)
		return 1.5 * sign;
	if (fabs(2 - scale) < 2 * 0.1)
		return 2 * sign;
	if (fabs(3 - scale) < 3 * 0.1)
		return 3 * sign;
	if (fabs(4 - scale) < 4 * 0.1)
		return 4 * sign;
	if (fabs(5 - scale) < 5 * 0.1)
		return 5 * sign;
	if (fabs(6 - scale) < 6 * 0.1)
		return 6 * sign;

	return scale * sign;
}

//------------------------------------------------------------------
// DragState

// constructor
DragState::DragState(TransformBox* parent)
	: fOrigin(0.0, 0.0),
	  fParent(parent)
{
}

// SetOrigin
void
DragState::SetOrigin(BPoint origin)
{
	fOrigin = origin;
}

// ActionName
const char*
DragState::ActionName() const
{
	return "Transformation";
}

// ActionNameIndex
uint32
DragState::ActionNameIndex() const
{
	return TRANSFORMATION;
}

// _SetViewCursor
void
DragState::_SetViewCursor(BView* view, const uchar* cursorData) const
{
	BCursor cursor(cursorData);
	view->SetViewCursor(&cursor);
}

// #pragma mark - DragCornerState

// This one works by transforming everything inversely
// then creating a transformation on the original box
// which is then applied *before* the already given
// transformation that was current when the drag started.

DragCornerState::DragCornerState(TransformBox* parent, uint32 corner)
	: DragState(parent),
	  fCorner(corner),
	  fOffsetFromCorner(B_ORIGIN),
	  fOldTransform()
{
}

// SetOrigin
void
DragCornerState::SetOrigin(BPoint origin)
{
	fOldTransform = *fParent;

	fOldTransform.InverseTransform(&origin);

	BRect box = fParent->Box();
	switch (fCorner) {
		case LEFT_TOP_CORNER:
			fOffsetFromCorner = origin - box.LeftTop();
			break;
		case RIGHT_TOP_CORNER:
			fOffsetFromCorner = origin - box.RightTop();
			break;
		case LEFT_BOTTOM_CORNER:
			fOffsetFromCorner = origin - box.LeftBottom();
			break;
		case RIGHT_BOTTOM_CORNER:
			fOffsetFromCorner = origin - box.RightBottom();
			break;
	}

	DragState::SetOrigin(origin);
}

// DragTo
void
DragCornerState::DragTo(BPoint current, uint32 modifiers)
{
	BRect oldBox = fParent->Box();
	if (oldBox.Width() == 0.0 || oldBox.Height() == 0.0)
		return;

	// TODO: some of this can be combined into less steps
	BRect newBox = oldBox;

	fOldTransform.InverseTransform(&current);
	
	switch (fCorner) {
		case LEFT_TOP_CORNER:
			newBox.left = current.x - fOffsetFromCorner.x;
			newBox.top = current.y - fOffsetFromCorner.y;
			break;
		case RIGHT_TOP_CORNER:
			newBox.right = current.x - fOffsetFromCorner.x;
			newBox.top = current.y - fOffsetFromCorner.y;
			break;
		case LEFT_BOTTOM_CORNER:
			newBox.left = current.x - fOffsetFromCorner.x;
			newBox.bottom = current.y - fOffsetFromCorner.y;
			break;
		case RIGHT_BOTTOM_CORNER:
			newBox.right = current.x - fOffsetFromCorner.x;
			newBox.bottom = current.y - fOffsetFromCorner.y;
			break;
	}

	if (!(modifiers & B_SHIFT_KEY)) {
		// keep the x and y scale the same
		double xScale = newBox.Width() / oldBox.Width();
		double yScale = newBox.Height() / oldBox.Height();
	
		if (modifiers & B_COMMAND_KEY) {
			xScale = snap_scale(xScale);
			yScale = snap_scale(yScale);
		}

		if (fabs(xScale) > fabs(yScale))
			yScale = yScale > 0.0 ? fabs(xScale) : -fabs(xScale);
		else
			xScale = xScale > 0.0 ? fabs(yScale) : -fabs(yScale);

		switch (fCorner) {
			case LEFT_TOP_CORNER:
				newBox.left = oldBox.right - oldBox.Width() * xScale;
				newBox.top = oldBox.bottom - oldBox.Height() * yScale;
				break;
			case RIGHT_TOP_CORNER:
				newBox.right = oldBox.left + oldBox.Width() * xScale;
				newBox.top = oldBox.bottom - oldBox.Height() * yScale;
				break;
			case LEFT_BOTTOM_CORNER:
				newBox.left = oldBox.right - oldBox.Width() * xScale;
				newBox.bottom = oldBox.top + oldBox.Height() * yScale;
				break;
			case RIGHT_BOTTOM_CORNER:
				newBox.right = oldBox.left + oldBox.Width() * xScale;
				newBox.bottom = oldBox.top + oldBox.Height() * yScale;
				break;
		}
	}

	// build a matrix that performs just the
	// distortion of the box with the opposite
	// corner of the one being dragged staying fixed
	AffineTransform s;
	s.rect_to_rect(oldBox.left, oldBox.top, oldBox.right, oldBox.bottom,
				   newBox.left, newBox.top, newBox.right, newBox.bottom);

	// construct a transformation that
	// * excludes the effect of the fParant->Pivot()
	// * includes the effect of the changed scaling and translation
	AffineTransform t;
	BPoint pivot(fParent->Pivot());
	t.TranslateBy(pivot.x, pivot.y);
	t.Multiply(s);
		// at this point, the matrix is
		// similar/compatible to the original
		// matrix (fOldTransform), and also
		// contains the pivot
	t.Multiply(fOldTransform);
		// here both matrices are "merged"
		// -> in effect this means that the
		// scale and the translation to
		// keep the object fixed at the corner
		// opposite to the one being dragged
		// were transfered to the parent matrix
	t.TranslateBy(-pivot.x, -pivot.y);
		// and now the pivot is removed
		// (see AdvancedTransform::_UpdateMatrix()
		// for how the pivot is applied)

	// get the translation
	double translationX;
	double translationY;
	t.translation(&translationX, &translationY);

	// get the scale
	double newScaleX;
	double newScaleY;
	t.scaling(&newScaleX, &newScaleY);

	// operating on just the affine parameters is much more precise
	fParent->SetTranslationAndScale(BPoint(translationX, translationY),
		newScaleX, newScaleY);
}

// UpdateViewCursor
void
DragCornerState::UpdateViewCursor(BView* view, BPoint current) const
{
	// TODO: doesn't work well for perspective transformations
	float rotation = fmod(360.0 - fParent->ViewSpaceRotation() + 22.5, 180.0);
	bool flipX = fParent->LocalXScale() < 0.0;
	bool flipY = fParent->LocalYScale() < 0.0;
	if (rotation < 45.0) {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kLeftTopRightBottomCursor
											   : kLeftBottomRightTopCursor);
				else
					_SetViewCursor(view, flipY ? kLeftBottomRightTopCursor
											   : kLeftTopRightBottomCursor);
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kLeftBottomRightTopCursor
											   : kLeftTopRightBottomCursor);
				else
					_SetViewCursor(view, flipY ? kLeftTopRightBottomCursor
											   : kLeftBottomRightTopCursor);
				break;
		}
	} else if (rotation < 90.0) {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kLeftRightCursor
											   : kUpDownCursor);
				else
					_SetViewCursor(view, flipY ? kUpDownCursor
											   : kLeftRightCursor);
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kUpDownCursor
											   : kLeftRightCursor);
				else
					_SetViewCursor(view, flipY ? kLeftRightCursor
											   : kUpDownCursor);
				break;
		}
	} else if (rotation < 135.0) {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kLeftBottomRightTopCursor
											   : kLeftTopRightBottomCursor);
				else
					_SetViewCursor(view, flipY ? kLeftTopRightBottomCursor
											   : kLeftBottomRightTopCursor);
				break;
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kLeftTopRightBottomCursor
											   : kLeftBottomRightTopCursor);
				else
					_SetViewCursor(view, flipY ? kLeftBottomRightTopCursor
											   : kLeftTopRightBottomCursor);
				break;
				break;
		}
	} else {
		switch (fCorner) {
			case LEFT_TOP_CORNER:
			case RIGHT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kUpDownCursor
											   : kLeftRightCursor);
				else
					_SetViewCursor(view, flipY ? kLeftRightCursor
											   : kUpDownCursor);
				break;
			case RIGHT_TOP_CORNER:
			case LEFT_BOTTOM_CORNER:
				if (flipX)
					_SetViewCursor(view, flipY ? kLeftRightCursor
											   : kUpDownCursor);
				else
					_SetViewCursor(view, flipY ? kUpDownCursor
											   : kLeftRightCursor);
				break;
		}
	}
}

// ActionName
const char*
DragCornerState::ActionName() const
{
	return "Scale";
}

// ActionNameIndex
uint32
DragCornerState::ActionNameIndex() const
{
	return SCALE;
}

// #pragma mark - DragSideState

DragSideState::DragSideState(TransformBox* parent, uint32 side)
	: DragState(parent),
	  fSide(side)
{
}

// SetOrigin
void
DragSideState::SetOrigin(BPoint origin)
{
	fOldTransform = *fParent;

	fOldTransform.InverseTransform(&origin);

	BRect box = fParent->Box();
	switch (fSide) {
		case LEFT_SIDE:
			fOffsetFromSide = origin.x - box.left;
			break;
		case RIGHT_SIDE:
			fOffsetFromSide = origin.x - box.right;
			break;
		case TOP_SIDE:
			fOffsetFromSide = origin.y - box.top;
			break;
		case BOTTOM_SIDE:
			fOffsetFromSide = origin.y - box.bottom;
			break;
	}

	DragState::SetOrigin(origin);
}

// DragTo
void
DragSideState::DragTo(BPoint current, uint32 modifiers)
{
	BRect oldBox = fParent->Box();
	if (oldBox.Width() == 0.0 || oldBox.Height() == 0.0)
		return;

	// TODO: some of this can be combined into less steps
	BRect newBox = oldBox;

	fOldTransform.InverseTransform(&current);
	
	switch (fSide) {
		case LEFT_SIDE:
			newBox.left = current.x - fOffsetFromSide;
			break;
		case RIGHT_SIDE:
			newBox.right = current.x - fOffsetFromSide;
			break;
		case TOP_SIDE:
			newBox.top = current.y - fOffsetFromSide;
			break;
		case BOTTOM_SIDE:
			newBox.bottom = current.y - fOffsetFromSide;
			break;
	}

	if (!(modifiers & B_SHIFT_KEY)) {
		// keep the x and y scale the same
		double xScale = newBox.Width() / oldBox.Width();
		double yScale = newBox.Height() / oldBox.Height();

		if (modifiers & B_COMMAND_KEY) {
			xScale = snap_scale(xScale);
			yScale = snap_scale(yScale);
		}

		if (fSide == LEFT_SIDE || fSide == RIGHT_SIDE)
			yScale = yScale > 0.0 ? fabs(xScale) : -fabs(xScale);
		else
			xScale = xScale > 0.0 ? fabs(yScale) : -fabs(yScale);

		switch (fSide) {
			case LEFT_SIDE: {
				newBox.left = oldBox.right - oldBox.Width() * xScale;
				float middle = (oldBox.top + oldBox.bottom) / 2.0;
				float newHeight = oldBox.Height() * yScale;
				newBox.top = middle - newHeight / 2.0;
				newBox.bottom = middle + newHeight / 2.0;
				break;
			}

			case RIGHT_SIDE: {
				newBox.right = oldBox.left + oldBox.Width() * xScale;
				float middle = (oldBox.top + oldBox.bottom) / 2.0;
				float newHeight = oldBox.Height() * yScale;
				newBox.top = middle - newHeight / 2.0;
				newBox.bottom = middle + newHeight / 2.0;
				break;
			}

			case TOP_SIDE: {
				newBox.top = oldBox.bottom - oldBox.Height() * yScale;
				float middle = (oldBox.left + oldBox.right) / 2.0;
				float newWidth = oldBox.Width() * xScale;
				newBox.left = middle - newWidth / 2.0;
				newBox.right = middle + newWidth / 2.0;
				break;
			}

			case BOTTOM_SIDE: {
				newBox.bottom = oldBox.top + oldBox.Height() * yScale;
				float middle = (oldBox.left + oldBox.right) / 2.0;
				float newWidth = oldBox.Width() * xScale;
				newBox.left = middle - newWidth / 2.0;
				newBox.right = middle + newWidth / 2.0;
				break;
			}
		}
	}

	// build a matrix that performs just the
	// distortion of the box with the opposite
	// corner of the one being dragged staying fixed
	AffineTransform s;
	s.rect_to_rect(oldBox.left, oldBox.top, oldBox.right, oldBox.bottom,
				   newBox.left, newBox.top, newBox.right, newBox.bottom);

	// construct a transformation that
	// * excludes the effect of the fParant->Pivot()
	// * includes the effect of the changed scaling and translation
	// (see DragCornerState::DragTo() for explaination)
	AffineTransform t;
	BPoint pivot(fParent->Pivot());
	t.TranslateBy(pivot.x, pivot.y);
	t.Multiply(s);
	t.Multiply(fOldTransform);
	t.TranslateBy(-pivot.x, -pivot.y);

	// get the translation
	double translationX;
	double translationY;
	t.translation(&translationX, &translationY);

	// get the scale
	double newScaleX;
	double newScaleY;
	t.scaling(&newScaleX, &newScaleY);

	// operating on just the affine parameters is much more precise
	fParent->SetTranslationAndScale(BPoint(translationX, translationY),
		newScaleX, newScaleY);
}

// UpdateViewCursor
void
DragSideState::UpdateViewCursor(BView* view, BPoint current) const
{
	float rotation = fmod(360.0 - fParent->ViewSpaceRotation() + 22.5, 180.0);
	if (rotation < 45.0) {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kLeftRightCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kUpDownCursor);
				break;
		}
	} else if (rotation < 90.0) {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kLeftBottomRightTopCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kLeftTopRightBottomCursor);
				break;
		}
	} else if (rotation < 135.0) {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kUpDownCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kLeftRightCursor);
				break;
		}
	} else {
		switch (fSide) {
			case LEFT_SIDE:
			case RIGHT_SIDE:
				_SetViewCursor(view, kLeftTopRightBottomCursor);
				break;
			case TOP_SIDE:
			case BOTTOM_SIDE:
				_SetViewCursor(view, kLeftBottomRightTopCursor);
				break;
		}
	}
}

// ActionName
const char*
DragSideState::ActionName() const
{
	return "Scale";
}

// ActionNameIndex
uint32
DragSideState::ActionNameIndex() const
{
	return SCALE;
}


// #pragma mark - DragBoxState

// SetOrigin
void
DragBoxState::SetOrigin(BPoint origin)
{
	fOldTranslation = fParent->Translation();
	DragState::SetOrigin(origin);
}

// DragTo
void
DragBoxState::DragTo(BPoint current, uint32 modifiers)
{
	BPoint offset = current - fOrigin;
	BPoint newTranslation = fOldTranslation + offset;
	if (modifiers & B_SHIFT_KEY) {
		if (fabs(offset.x) > fabs(offset.y))
			newTranslation.y = fOldTranslation.y;
		else
			newTranslation.x = fOldTranslation.x;
	}
	fParent->TranslateBy(newTranslation - fParent->Translation());
}

// UpdateViewCursor
void
DragBoxState::UpdateViewCursor(BView* view, BPoint current) const
{
	_SetViewCursor(view, kMoveCursor);
}

// ActionName
const char*
DragBoxState::ActionName() const
{
	return "Move";
}

// ActionNameIndex
uint32
DragBoxState::ActionNameIndex() const
{
	return MOVE;
}


// #pragma mark - RotateBoxState

// constructor
RotateBoxState::RotateBoxState(TransformBox* parent)
	: DragState(parent),
	  fOldAngle(0.0)
{
}

// SetOrigin
void
RotateBoxState::SetOrigin(BPoint origin)
{
	DragState::SetOrigin(origin);
	fOldAngle = fParent->LocalRotation();
}

// DragTo
void
RotateBoxState::DragTo(BPoint current, uint32 modifiers)
{
	double angle = calc_angle(fParent->Origin(), fOrigin, current);

	double newAngle = fOldAngle + angle;

	if (modifiers & B_SHIFT_KEY) {
		if (newAngle < 0.0)
			newAngle -= 22.5;
		else
			newAngle += 22.5;
		newAngle = 45.0 * ((int32)newAngle / 45);
	}

	fParent->RotateBy(newAngle - fParent->LocalRotation());
}

// UpdateViewCursor
void
RotateBoxState::UpdateViewCursor(BView* view, BPoint current) const
{
	BPoint origin(fParent->Origin());
	fParent->TransformFromCanvas(origin);
	fParent->TransformFromCanvas(current);
	BPoint from = origin + BPoint(sinf(22.5 * 180.0 / M_PI) * 50.0,
								  -cosf(22.5 * 180.0 / M_PI) * 50.0);

	float rotation = calc_angle(origin, from, current) + 180.0;

	if (rotation < 45.0) {
		_SetViewCursor(view, kRotateLCursor);
	} else if (rotation < 90.0) {
		_SetViewCursor(view, kRotateLTCursor);
	} else if (rotation < 135.0) {
		_SetViewCursor(view, kRotateTCursor);
	} else if (rotation < 180.0) {
		_SetViewCursor(view, kRotateRTCursor);
	} else if (rotation < 225.0) {
		_SetViewCursor(view, kRotateRCursor);
	} else if (rotation < 270.0) {
		_SetViewCursor(view, kRotateRBCursor);
	} else if (rotation < 315.0) {
		_SetViewCursor(view, kRotateBCursor);
	} else {
		_SetViewCursor(view, kRotateLBCursor);
	}
}

// ActionName
const char*
RotateBoxState::ActionName() const
{
	return "Rotate";
}

// ActionNameIndex
uint32
RotateBoxState::ActionNameIndex() const
{
	return ROTATE;
}



// #pragma mark - OffsetCenterState

// SetOrigin
void
OffsetCenterState::SetOrigin(BPoint origin)
{
	fParent->InverseTransform(&origin);
	DragState::SetOrigin(origin);
}


// DragTo
void
OffsetCenterState::DragTo(BPoint current, uint32 modifiers)
{
	// apply the pivot offset on a temporary copy,
	// calculate the effect on the translation and compensate it

	fParent->InverseTransform(&current);

	AdvancedTransform t(*fParent);

	BPoint originA(B_ORIGIN);
	t.Transform(&originA);

	t.SetPivot(t.Pivot() + (current - fOrigin));

	BPoint originB(B_ORIGIN);
	t.Transform(&originB);

	originA = originA - originB;

	fParent->SetTransformation(t.Pivot(),
							   t.Translation() + originA,
							   t.LocalRotation(),
							   t.LocalXScale(),
							   t.LocalYScale());

	fOrigin = current;
}

// UpdateViewCursor
void
OffsetCenterState::UpdateViewCursor(BView* view, BPoint current) const
{
	_SetViewCursor(view, kPathMoveCursor);
}

// ActionName
const char*
OffsetCenterState::ActionName() const
{
	return "Move Pivot";
}

// ActionNameIndex
uint32
OffsetCenterState::ActionNameIndex() const
{
	return MOVE_PIVOT;
}



/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AdvancedTransform.h"

#include <stdio.h>

// constructor
AdvancedTransform::AdvancedTransform()
	: AffineTransform(),
	  fPivot(0.0, 0.0),
	  fTranslation(0.0, 0.0),
	  fRotation(0.0),
	  fXScale(1.0),
	  fYScale(1.0)
{
}

// copy constructor
AdvancedTransform::AdvancedTransform(const AdvancedTransform& other)
	: AffineTransform(other),
	  fPivot(other.fPivot),
	  fTranslation(other.fTranslation),
	  fRotation(other.fRotation),
	  fXScale(other.fXScale),
	  fYScale(other.fYScale)
{
}

// destructor
AdvancedTransform::~AdvancedTransform()
{
}

// SetTransformation
void
AdvancedTransform::SetTransformation(BPoint pivot,
									 BPoint translation,
									 double rotation,
									 double xScale,
									 double yScale)
{
	if (fTranslation != translation ||
		fPivot != pivot ||
		fRotation != rotation ||
		fXScale != xScale ||
		fYScale != yScale) {

		fPivot = pivot;
		fTranslation = translation;
		fRotation = rotation;
		fXScale = xScale;
		fYScale = yScale;
	
		_UpdateMatrix();
	}
}

// UpdateAffineParamsFromMatrix
void
AdvancedTransform::UpdateAffineParamsFromMatrix()
{
	
}

// SetPivot
void
AdvancedTransform::SetPivot(BPoint pivot)
{
	if (pivot == fPivot)
		return;

	fPivot = pivot;

	_UpdateMatrix();
}

// TranslateBy
void
AdvancedTransform::TranslateBy(BPoint offset)
{
	if (offset.x == 0.0 && offset.y == 0.0)
		return;

	fTranslation += offset;

	_UpdateMatrix();
}

// TranslateBy
void
AdvancedTransform::TranslateBy(double x, double y)
{
	if (x == 0.0 && y == 0.0)
		return;

	fTranslation.x += x;
	fTranslation.y += y;

	_UpdateMatrix();
}

// RotateBy
//
// converts a rotation in world coordinates into
// a combined local rotation and a translation
void
AdvancedTransform::RotateBy(BPoint origin, double degrees)
{
	if (degrees == 0.0)
		return;

	origin -= fPivot;

	fRotation += degrees;

	// rotate fTranslation
	double xOffset = fTranslation.x - origin.x;
	double yOffset = fTranslation.y - origin.y;

	agg::trans_affine_rotation m(degrees * M_PI / 180.0);
	m.transform(&xOffset, &yOffset);

	fTranslation.x = origin.x + xOffset;
	fTranslation.y = origin.y + yOffset;

	_UpdateMatrix();
}


// RotateBy
void
AdvancedTransform::RotateBy(double degrees)
{
	if (degrees == 0.0)
		return;

	fRotation += degrees;

	_UpdateMatrix();
}

//// ScaleBy
////
//// converts a scalation in world coordinates into
//// a combined local scalation and a translation
//void
//AdvancedTransform::ScaleBy(BPoint origin, double xScale, double yScale)
//{
//	if (xScale == 1.0 && yScale == 1.0)
//		return;
//
//	fXScale *= xScale;
//	fYScale *= yScale;
//
//	// scale fTranslation
//	double xOffset = fTranslation.x - origin.x;
//	double yOffset = fTranslation.y - origin.y;
//
//	fTranslation.x = origin.x + (xOffset * xScale);
//	fTranslation.y = origin.y + (yOffset * yScale);
//
//	_UpdateMatrix();
//}

// ScaleBy
void
AdvancedTransform::ScaleBy(double xScale, double yScale)
{
	if (xScale == 1.0 && yScale == 1.0)
		return;

	fXScale *= xScale;
	fYScale *= yScale;

	_UpdateMatrix();
}

// SetTranslationAndScale
void
AdvancedTransform::SetTranslationAndScale(BPoint offset,
										  double xScale, double yScale)
{
	if (fTranslation == offset && fXScale == xScale && fYScale == yScale)
		return;

	fTranslation = offset;

	fXScale = xScale;
	fYScale = yScale;

	_UpdateMatrix();
}

// Reset
void
AdvancedTransform::Reset()
{
	SetTransformation(B_ORIGIN, B_ORIGIN, 0.0, 1.0, 1.0);
}

// =
AdvancedTransform&
AdvancedTransform::operator=(const AdvancedTransform& other)
{
	fTranslation = other.fTranslation;
	fRotation = other.fRotation;
	fXScale = other.fXScale;
	fYScale = other.fYScale;

	AffineTransform::operator=(other);

	return *this;
}

// PrintToStream
void
AdvancedTransform::PrintToStream()
{
	printf("AdvancedTransform(\n");
	printf("        pivot: %.2f, %.2f\n", fPivot.x, fPivot.y);
	printf("  translation: %.2f, %.2f\n", fTranslation.x, fTranslation.y);
	printf("     rotation: %.2f\n", fRotation);
	printf("        scale: %.2f, %.2f\n", fXScale, fYScale);
}

// #pragma mark -

// _UpdateMatrix
void
AdvancedTransform::_UpdateMatrix()
{
	// fix up scales in case any is zero
	double xScale = fXScale;
	if (xScale == 0.0)
		xScale = 0.000001;
	double yScale = fYScale;
	if (yScale == 0.0)
		yScale = 0.000001;

	// start clean
	reset();
	// the "pivot" is like the offset from world to local
	// coordinate system and is the center for rotation and scale
	multiply(agg::trans_affine_translation(-fPivot.x, -fPivot.y));
	multiply(agg::trans_affine_scaling(xScale, yScale));
	multiply(agg::trans_affine_rotation(fRotation * M_PI / 180.0));

	multiply(agg::trans_affine_translation(fPivot.x + fTranslation.x,
										   fPivot.y + fTranslation.y));

	// call hook function
	Update();
}


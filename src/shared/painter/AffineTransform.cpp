/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AffineTransform.h"

#include <stdio.h>
#include <string.h>

#include "support.h"

// constructor
AffineTransform::AffineTransform()
	: agg::trans_affine()
{
}

// copy constructor
AffineTransform::AffineTransform(const AffineTransform& other)
	: agg::trans_affine(other)
{
}

// destructor
AffineTransform::~AffineTransform()
{
}

// StoreTo
void
AffineTransform::StoreTo(double matrix[6]) const
{
	store_to(matrix);
}

// LoadFrom
void
AffineTransform::LoadFrom(double matrix[6])
{
	// before calling the potentially heavy Notify()
	// hook function, make sure that the transformation
	// really changed
	AffineTransform t;
	t.load_from(matrix);
	if (*this != t) {
		load_from(matrix);
		Notify();
	}
}

// SetTransform
void
AffineTransform::SetTransform(const AffineTransform& other)
{
	if (*this != other) {
		*this = other;
		Notify();
	}
}

// operator=
AffineTransform&
AffineTransform::operator=(const AffineTransform& other)
{
	if (other != *this) {
		reset();
		multiply(other);
		Notify();
	}
	return *this;
}

// Multiply
AffineTransform&
AffineTransform::Multiply(const AffineTransform& other)
{
	if (!other.IsIdentity()) {
		multiply(other);
		Notify();
	}
	return *this;
}

// Reset
void
AffineTransform::Reset()
{
	reset();
}

// Invert
void
AffineTransform::Invert()
{
	invert();
}

// IsIdentity
bool
AffineTransform::IsIdentity() const
{
	double m[6];
	store_to(m);
	if (m[0] == 1.0 &&
		m[1] == 0.0 &&
		m[2] == 0.0 &&
		m[3] == 1.0 &&
		m[4] == 0.0 &&
		m[5] == 0.0)
		return true;
	return false;
}

// IsTranslationOnly
bool
AffineTransform::IsTranslationOnly() const
{
	double m[6];
	store_to(m);
	if (m[0] == 1.0 &&
		m[1] == 0.0 &&
		m[2] == 0.0 &&
		m[3] == 1.0)
		return true;
	return false;
}

// IsNotDistorted
bool
AffineTransform::IsNotDistorted() const
{
	double m[6];
	store_to(m);
	return (m[0] == m[3]);
}

// IsValid
bool
AffineTransform::IsValid() const
{
	double m[6];
	store_to(m);
	return ((m[0] * m[3] - m[1] * m[2]) != 0.0);
}

// operator==
bool
AffineTransform::operator==(const AffineTransform& other) const
{
	double m1[6];
	other.store_to(m1);
	double m2[6];
	store_to(m2);
	if (m1[0] == m2[0] &&
		m1[1] == m2[1] &&
		m1[2] == m2[2] &&
		m1[3] == m2[3] &&
		m1[4] == m2[4] &&
		m1[5] == m2[5])
		return true;
	return false;
}

// operator!=
bool
AffineTransform::operator!=(const AffineTransform& other) const
{
	return !(*this == other);
}

// Transform
void
AffineTransform::Transform(double* x, double* y) const
{
	transform(x, y);
}

// Transform
void
AffineTransform::Transform(BPoint* point) const
{
	if (point) {
		double x = point->x;
		double y = point->y;

		transform(&x, &y);
	
		point->x = x;
		point->y = y;
	}
}

// Transform
BPoint
AffineTransform::Transform(const BPoint& point) const
{
	BPoint p(point);
	Transform(&p);
	return p;
}

// InverseTransform
void
AffineTransform::InverseTransform(double* x, double* y) const
{
	inverse_transform(x, y);
}

// InverseTransform
void
AffineTransform::InverseTransform(BPoint* point) const
{
	if (point) {
		double x = point->x;
		double y = point->y;

		inverse_transform(&x, &y);
	
		point->x = x;
		point->y = y;
	}
}

// InverseTransform
BPoint
AffineTransform::InverseTransform(const BPoint& point) const
{
	BPoint p(point);
	InverseTransform(&p);
	return p;
}

// TransformBounds
BRect
AffineTransform::TransformBounds(BRect bounds) const
{
	if (bounds.IsValid()) {
		BPoint lt(bounds.left, bounds.top);
		BPoint rt(bounds.right, bounds.top);
		BPoint lb(bounds.left, bounds.bottom);
		BPoint rb(bounds.right, bounds.bottom);
	
		Transform(&lt);
		Transform(&rt);
		Transform(&lb);
		Transform(&rb);
	
		return BRect(floorf(min4(lt.x, rt.x, lb.x, rb.x)),
					 floorf(min4(lt.y, rt.y, lb.y, rb.y)),
					 ceilf(max4(lt.x, rt.x, lb.x, rb.x)),
					 ceilf(max4(lt.y, rt.y, lb.y, rb.y)));
	}
	return bounds;
}

// TranslateBy
void
AffineTransform::TranslateBy(BPoint offset)
{
	if (offset.x != 0.0 || offset.y != 0.0) {
		multiply(agg::trans_affine_translation(offset.x, offset.y));
		Notify();
	}
}

// TranslateBy
void
AffineTransform::TranslateBy(double x, double y)
{
	if (x != 0.0 || y != 0.0) {
		multiply(agg::trans_affine_translation(x, y));
		Notify();
	}
}

// RotateBy
void
AffineTransform::RotateBy(BPoint origin, double degrees)
{
	if (degrees != 0.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_rotation(degrees * (M_PI / 180.0)));
		multiply(agg::trans_affine_translation(origin.x, origin.y));
		Notify();
	}
}

// ScaleBy
void
AffineTransform::ScaleBy(BPoint origin, double xScale, double yScale)
{
	if (xScale != 1.0 || yScale != 1.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_scaling(xScale, yScale));
		multiply(agg::trans_affine_translation(origin.x, origin.y));
		Notify();
	}
}

// ShearBy
void
AffineTransform::ShearBy(BPoint origin, double xShear, double yShear)
{
	if (xShear != 0.0 || yShear != 0.0) {
		multiply(agg::trans_affine_translation(-origin.x, -origin.y));
		multiply(agg::trans_affine_skewing(xShear, yShear));
		multiply(agg::trans_affine_translation(origin.x, origin.y));
		Notify();
	}
}

// #pragma mark -

// ScaleBy
void
AffineTransform::ScaleBy(double scale)
{
	ScaleBy(B_ORIGIN, scale, scale);
}

// ScaleBy
void
AffineTransform::ScaleBy(double xScale, double yScale)
{
	ScaleBy(B_ORIGIN, xScale, yScale);
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef AFFINE_TRANSFORM_H
#define AFFINE_TRANSFORM_H

#include <Rect.h>

#include <agg_trans_affine.h>

#include "Observable.h"

class AffineTransform : public Observable,
						public agg::trans_affine {
 public:
								AffineTransform();
								AffineTransform(const AffineTransform& other);
	virtual						~AffineTransform();

			void				StoreTo(double matrix[6]) const;
			void				LoadFrom(double matrix[6]);

								// set to or combine with other matrix
			void				SetTransform(const AffineTransform& other);
			AffineTransform&	operator=(const AffineTransform& other);
			AffineTransform&	Multiply(const AffineTransform& other);
	virtual	void				Reset();

			void				Invert();

			bool				IsIdentity() const;
			bool				IsTranslationOnly() const;
			bool				IsNotDistorted() const;
			bool				IsValid() const;

			bool				operator==(const AffineTransform& other) const;
			bool				operator!=(const AffineTransform& other) const;

								// transforms coordiantes
			void				Transform(double* x, double* y) const;
			void				Transform(BPoint* point) const;
			BPoint				Transform(const BPoint& point) const;

			void				InverseTransform(double* x, double* y) const;
			void				InverseTransform(BPoint* point) const;
			BPoint				InverseTransform(const BPoint& point) const;

								// transforms the rectangle "bounds" and
								// returns the *bounding box* of that
			BRect				TransformBounds(BRect bounds) const;

								// some convenience functions
	virtual	void				TranslateBy(BPoint offset);
	virtual	void				TranslateBy(double x, double y);
	virtual	void				RotateBy(BPoint origin, double degrees);
	virtual	void				ScaleBy(BPoint origin, double xScale, double yScale);
	virtual	void				ShearBy(BPoint origin, double xShear, double yShear);

			void				ScaleBy(double scale);
			void				ScaleBy(double xScale, double yScale);
};

#endif // AFFINE_TRANSFORM_H


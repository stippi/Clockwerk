/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef ADVANCED_TRANSFORM_H
#define ADVANCED_TRANSFORM_H

#include "AffineTransform.h"

class AdvancedTransform : public AffineTransform {
 public:
								AdvancedTransform();
								AdvancedTransform(const AdvancedTransform& other);
	virtual						~AdvancedTransform();

	virtual	void				Update(bool deep = true) {}

			void				SetTransformation(BPoint pivot,
												  BPoint translation,
												  double rotation,
												  double xScale,
												  double yScale);

			void				SetPivot(BPoint pivot);

			void				UpdateAffineParamsFromMatrix();

	virtual	void				TranslateBy(BPoint offset);
	virtual	void				TranslateBy(double x, double y);
	virtual	void				RotateBy(BPoint origin, double degrees);

			void				ScaleBy(double xScale, double yScale);
			void				RotateBy(double degrees);

			void				SetTranslationAndScale(BPoint offset,
													   double xScale, double yScale);

	virtual	void				Reset();

	inline	BPoint				Pivot() const
									{ return fPivot; }
	inline	BPoint				Translation() const
									{ return fTranslation; }
	inline	double				LocalRotation() const
									{ return fRotation; }
	inline	double				LocalXScale() const
									{ return fXScale; }
	inline	double				LocalYScale() const
									{ return fYScale; }

			AdvancedTransform&	operator=(const AdvancedTransform& other);

			void				PrintToStream();
 protected:
			void				_UpdateMatrix();

			BPoint				fPivot;
			BPoint				fTranslation;
			double				fRotation;
			double				fXScale;
			double				fYScale;
};

#endif // ADVANCED_TRANSFORM_H


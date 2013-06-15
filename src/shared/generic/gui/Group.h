/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef GROUP_H
#define GROUP_H

#include <View.h>

class Group : public BView {
 public:
								Group(BRect frame,
									  const char* name,
									  orientation direction = B_HORIZONTAL);
	virtual						~Group();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				GetPreferredSize(float* width, float* height);

	// TODO: allow setting inset and spacing

 private:
			void				_LayoutControls(BRect frame) const;
			BRect				_MinFrame() const;
			void				_LayoutControl(BView* view,
											   BRect frame,
											   bool resizeWidth = false,
											   bool resizeHeight = false) const;

			orientation			fOrientation;
			float				fInset;
			float				fSpacing;
};

#endif	// GROUP_H

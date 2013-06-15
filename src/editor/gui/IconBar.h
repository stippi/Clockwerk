/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ICON_BAR_H
#define ICON_BAR_H

#include <View.h>

class IconBar : public BView {
 public:
								IconBar(BRect frame,
										enum orientation orientation = B_HORIZONTAL);
	virtual						~IconBar();

	// BView interface
	virtual	void				AllAttached();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);

 private:
			BRect				fPreviousBounds;
			enum orientation	fOrientation;
};

#endif // ICON_BAR_H

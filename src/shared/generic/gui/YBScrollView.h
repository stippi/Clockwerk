/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef YB_SCROLL_VIEW_H
#define YB_SCROLL_VIEW_H

#include <ScrollView.h>

class YBScrollView : public BScrollView {
 public:
								YBScrollView(const char* name, BView* target,
									uint32 resizeMask = B_FOLLOW_LEFT
										| B_FOLLOW_LEFT,
									uint32 flags = 0,
									bool horizontal = false,
									bool vertical = false,
									border_style border = B_FANCY_BORDER);


	// BView interface
	virtual	void				AttachedToWindow();

	virtual void				WindowActivated(bool state);
	virtual	void				Draw(BRect updateRect);

 private:
			bool				fWindowActive;

};

#endif // YB_SCROLL_VIEW_H

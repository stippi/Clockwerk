/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2007, Stephan Aßmus, superstippi@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#ifndef WEEK_MENU_ITEM_H
#define WEEK_MENU_ITEM_H

#include <MenuItem.h>

class WeekMenuItem : public BMenuItem {
 public:
								WeekMenuItem(BMessage* message,
									int32 week, int32 month, int32 year);
	virtual						~WeekMenuItem();

	virtual void				DrawContent();
	virtual void				GetContentSize(float* _width, float* _height);

 private:
			int32				fWeek;
			int32				fMonth;
			int32				fYear;

			float				fTitleHeight;
			float				fRowHeight;
			float				fColumnWidth;
			float				fFontHeight;

			int32				fFirstWeekday;
};

#endif // WEEK_MENU_ITEM_H

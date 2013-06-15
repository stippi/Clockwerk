/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef INPUT_SLIDER_H
#define INPUT_SLIDER_H

#include "PopupSlider.h"

class NummericalTextView;
class BMessageFilter;

class InputSlider : public PopupSlider {
 public:
								InputSlider(const char* name = NULL,
											const char* label = NULL,
											BMessage* model = NULL,
											BHandler* target = NULL,
											int32 min = 0,
											int32 max = 100,
											int32 value = 0,
											const char* formatString = "%ld");
	virtual						~InputSlider();

								// MView
	virtual	minimax				layoutprefs();
	virtual	BRect				layout(BRect frame);

								// BView
	virtual	void				MouseDown(BPoint where);

								// PopupSlider
			void				SetEnabled(bool enabled);
								// override this to take some action
	virtual	void				ValueChanged(int32 newValue);
	virtual	void				DrawSlider(BRect frame, bool enabled);

 private:

			NummericalTextView*	fTextView;
			BMessageFilter*		fTextViewFilter;
};

#endif	// INPUT_SLIDER_H

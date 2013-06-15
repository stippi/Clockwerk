/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef LIST_LABEL_VIEW_H
#define LIST_LABEL_VIEW_H

#include <String.h>
#include <View.h>

// NOTE: can be used above a list view to label the
// kind of elements in the list

class ListLabelView : public BView {
 public:
								ListLabelView(BRect frame,
									const char* name, uint32 followMode,
									const char* label = NULL);
	virtual						~ListLabelView();

	// BView interface
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);
	virtual	void				GetPreferredSize(float* width,
									float* height);

	// ListLabelView
			void				SetLabel(const char* label);

 private:
			BString				fLabel;
			float				fOldWidth;
};

#endif // LIST_LABEL_VIEW_H

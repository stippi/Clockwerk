/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SWATCH_VIEW_H
#define SWATCH_VIEW_H

#include <View.h>

enum {
	MSG_COLOR_DROP				= 'PSTE',
};

class SwatchView : public BView {
public:
								SwatchView(const char* name, BMessage* message,
									BHandler* target, rgb_color color,
									float width = 24.0f, float height = 24.0f);
	virtual						~SwatchView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();


	// SwatchView
			void				SetColor(rgb_color color);
	inline	rgb_color			Color() const
									{ return fColor; }

			void				SetClickedMessage(BMessage* message);
			void				SetDroppedMessage(BMessage* message);

private:
			void				_Invoke(const BMessage* message);
			void				_StrokeRect(BRect frame, rgb_color leftTop,
									rgb_color rightBottom);
			void				_DragColor();

			rgb_color			fColor;
			BPoint				fTrackingStart;
			bool				fActive;
			bool				fDropInvokes;

			BMessage*			fClickMessage;
			BMessage*			fDroppedMessage;
			BHandler*			fTarget;

			float				fWidth;
			float				fHeight;
};

#endif // SWATCH_VIEW_H

/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef FONT_VALUE_VIEW_H
#define FONT_VALUE_VIEW_H

#include <String.h>

#include "PropertyEditorView.h"

class BMenu;
class FontProperty;

class FontValueView : public PropertyEditorView {
 public:
								FontValueView(FontProperty* property);
	virtual						~FontValueView();

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	virtual	void				SetEnabled(bool enabled);

	virtual	void				ValueChanged(Property* property);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			void				_PolulateMenu(BMenu* menu, BHandler* target,
											  const char* markedFamily,
											  const char* markedStyle);

			FontProperty*		fProperty;

			BString				fCurrentFont;
			bool				fEnabled;
};

#endif // OPTION_VALUE_VIEW_H



/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef COLOR_VALUE_VIEW_H
#define COLOR_VALUE_VIEW_H

#include "ColorProperty.h"
#include "PropertyEditorView.h"
#include "selected_color_mode.h"

#include <Locker.h>

class ColorPickerPanel;
class SwatchValueView;

class ColorValueView : public PropertyEditorView {
 public:
								ColorValueView(ColorProperty* property);
	virtual						~ColorValueView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused);

	virtual	void				MessageReceived(BMessage* message);

	// PropertyEditorView interface
	virtual	void				SetEnabled(bool enabled);
	virtual	bool				IsFocused() const;

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 protected:
			ColorProperty*		fProperty;

			SwatchValueView*	fSwatchView;

	static	ColorPickerPanel*	sColorPicker;
	static	BRect				sColorPickerFrame;
	static	selected_color_mode	sColorPickerMode;
	static	BLocker				sColorPickerSetupLock;
};

#endif // COLOR_VALUE_VIEW_H



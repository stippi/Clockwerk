/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TEXT_INPUT_VALUE_VIEW_H
#define TEXT_INPUT_VALUE_VIEW_H

#include "PropertyEditorView.h"

class InputTextView;

// Common base class for
// IntValueView,
// Int64ValueView,
// FloatValueView,
// StringValueView

class TextInputValueView : public PropertyEditorView {
 public:
								TextInputValueView();
	virtual						~TextInputValueView();

	// BView interface
	virtual	void				AttachedToWindow();

	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused);

	virtual	void				MessageReceived(BMessage* message);


	// PropertyItemValueView interface
	virtual	void				SetEnabled(bool enabled);
	virtual	bool				IsFocused() const;

	// TextInputValueView
	virtual	InputTextView*		TextView() const = 0;

	virtual	void				UpdateProperty() = 0;
};

#endif // TEXT_INPUT_VALUE_VIEW_H



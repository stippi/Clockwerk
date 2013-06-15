/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef STRING_VALUE_VIEW_H
#define STRING_VALUE_VIEW_H

#include "OptionProperty.h"
#include "Property.h"
#include "TextInputValueView.h"

class StringTextView;

class StringValueView : public TextInputValueView {
 public:
								StringValueView(StringProperty* property);
	virtual						~StringValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	virtual	void				UpdateProperty();

	// PropertyEditorView interface
	virtual	void				ValueChanged(Property* property);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			StringProperty*		fProperty;
			StringTextView*		fTextView;
};

#endif // STRING_VALUE_VIEW_H



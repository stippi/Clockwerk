/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef FLOAT_VALUE_VIEW_H
#define FLOAT_VALUE_VIEW_H

#include "Property.h"
#include "TextInputValueView.h"

class NummericalTextView;

class FloatValueView : public TextInputValueView {
 public:
								FloatValueView(FloatProperty* property);
	virtual						~FloatValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	virtual	void				UpdateProperty();

	// PropertyEditorView interface
	virtual	void				ValueChanged(Property* property);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			FloatProperty*		fProperty;
			NummericalTextView*	fTextView;
};

#endif // FLOAT_VALUE_VIEW_H



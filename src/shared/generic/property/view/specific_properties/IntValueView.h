/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef INT_VALUE_VIEW_H
#define INT_VALUE_VIEW_H

#include "Property.h"
#include "TextInputValueView.h"

class NummericalTextView;

class IntValueView : public TextInputValueView {
 public:
								IntValueView(IntProperty* property);
	virtual						~IntValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	virtual	void				UpdateProperty();

	// PropertyEditorView interface
	virtual	void				ValueChanged(Property* property);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			IntProperty*		fProperty;
			NummericalTextView*	fTextView;
};

#endif // INT_VALUE_VIEW_H



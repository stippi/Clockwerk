/*
 * Copyright 2007, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TIME_VALUE_VIEW_H
#define TIME_VALUE_VIEW_H

#include "TimeProperty.h"
#include "TextInputValueView.h"

class StringTextView;

class TimeValueView : public TextInputValueView {
 public:
								TimeValueView(TimeProperty* property);
	virtual						~TimeValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	virtual	void				UpdateProperty();

	// PropertyEditorView interface
	virtual	void				ValueChanged(Property* property);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			TimeProperty*		fProperty;
			StringTextView*		fTextView;
};

#endif // TIME_VALUE_VIEW_H



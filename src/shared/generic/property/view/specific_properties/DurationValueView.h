/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */

#ifndef DURATION_VALUE_VIEW_H
#define DURATION_VALUE_VIEW_H

#include "DurationProperty.h"
#include "TextInputValueView.h"

class StringTextView;

class DurationValueView : public TextInputValueView {
 public:
								DurationValueView(DurationProperty* property);
	virtual						~DurationValueView();

	// TextInputValueView interface
	virtual	InputTextView*		TextView() const;

	virtual	void				UpdateProperty();

	// PropertyEditorView interface
	virtual	void				ValueChanged(Property* property);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			DurationProperty*	fProperty;
			StringTextView*		fTextView;
};

#endif // DURATION_VALUE_VIEW_H



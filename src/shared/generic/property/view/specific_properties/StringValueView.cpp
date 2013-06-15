/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StringValueView.h"

#include <stdio.h>
#include <string.h>

#include "StringTextView.h"

// constructor
StringValueView::StringValueView(StringProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new StringTextView(b, "string input", b,
								   B_FOLLOW_LEFT | B_FOLLOW_TOP,
								   B_WILL_DRAW);
	AddChild(fTextView);

	if (fProperty)
		fTextView->SetValue(fProperty->Value());
}

// destructor
StringValueView::~StringValueView()
{
}

// TextView
InputTextView*
StringValueView::TextView() const
{
	return fTextView;
}

// UpdateProperty
void
StringValueView::UpdateProperty()
{
	if (fProperty && fProperty->SetValue(fTextView->Value()))
		ValueChanged(fProperty);
}

// ValueChanged
void
StringValueView::ValueChanged(Property* property)
{
//	if (fProperty && fProperty->SetValue(fTextView->Value()))
	if (fProperty)
		fTextView->SetValue(fProperty->Value());
	TextInputValueView::ValueChanged(property);
}

// AdoptProperty
bool
StringValueView::AdoptProperty(Property* property)
{
	StringProperty* p = dynamic_cast<StringProperty*>(property);
	if (p) {
		if (!fProperty || strcmp(p->Value(), fTextView->Text()) != 0) {
			fTextView->SetValue(p->Value());
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
StringValueView::GetProperty() const
{
	return fProperty;
}

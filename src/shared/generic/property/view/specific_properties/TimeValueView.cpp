/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "TimeValueView.h"

#include <stdio.h>

#include "support.h"

#include "StringTextView.h"

// constructor
TimeValueView::TimeValueView(TimeProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new StringTextView(b, "time input", b,
								   B_FOLLOW_LEFT | B_FOLLOW_TOP,
								   B_WILL_DRAW);
	AddChild(fTextView);

	if (fProperty) {
		BString value;
		fProperty->GetValue(value);
		fTextView->SetValue(value.String());
	}
}

// destructor
TimeValueView::~TimeValueView()
{
}

// TextView
InputTextView*
TimeValueView::TextView() const
{
	return fTextView;
}

// UpdateProperty
void
TimeValueView::UpdateProperty()
{
	if (fProperty && fProperty->SetValue(fTextView->Value()))
		ValueChanged(fProperty);
}

// ValueChanged
void
TimeValueView::ValueChanged(Property* property)
{
//	if (fProperty && fProperty->SetValue(fTextView->Value())) {
	if (fProperty) {
		BString value;
		fProperty->GetValue(value);
		fTextView->SetValue(value.String());
	}
	TextInputValueView::ValueChanged(property);
}

// AdoptProperty
bool
TimeValueView::AdoptProperty(Property* property)
{
	TimeProperty* p = dynamic_cast<TimeProperty*>(property);
	if (p) {
		BString value;
		p->GetValue(value);
		if (!fProperty || strcmp(value.String(), fTextView->Text()) != 0) {
			fTextView->SetValue(value.String());
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
TimeValueView::GetProperty() const
{
	return fProperty;
}




/*
 * Copyright 2006-2008, Stephan Aßmus <superstippi@gmx.de>.
 * Distributed under the terms of the MIT License.
 */

#include "DurationValueView.h"

#include <stdio.h>

#include "support.h"

#include "StringTextView.h"

// constructor
DurationValueView::DurationValueView(DurationProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new StringTextView(b, "duration input", b,
		B_FOLLOW_LEFT | B_FOLLOW_TOP, B_WILL_DRAW);
	AddChild(fTextView);

	if (fProperty) {
		BString value;
		fProperty->GetValue(value);
		fTextView->SetValue(value.String());
	}
}

// destructor
DurationValueView::~DurationValueView()
{
}

// TextView
InputTextView*
DurationValueView::TextView() const
{
	return fTextView;
}

// UpdateProperty
void
DurationValueView::UpdateProperty()
{
	if (fProperty && fProperty->SetValue(fTextView->Value()))
		ValueChanged(fProperty);
}

// ValueChanged
void
DurationValueView::ValueChanged(Property* property)
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
DurationValueView::AdoptProperty(Property* property)
{
	DurationProperty* p = dynamic_cast<DurationProperty*>(property);
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
DurationValueView::GetProperty() const
{
	return fProperty;
}




/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FloatValueView.h"

#include <stdio.h>

#include "support.h"

#include "NummericalTextView.h"

// constructor
FloatValueView::FloatValueView(FloatProperty* property)
	: TextInputValueView(),
	  fProperty(property)
{
	BRect b = Bounds();
	fTextView = new NummericalTextView(b, "nummerical input", b,
									   B_FOLLOW_LEFT | B_FOLLOW_TOP,
									   B_WILL_DRAW);

	AddChild(fTextView);
	fTextView->SetFloatMode(true);

	if (fProperty)
		fTextView->SetValue(fProperty->Value());
}

// destructor
FloatValueView::~FloatValueView()
{
}

// TextView
InputTextView*
FloatValueView::TextView() const
{
	return fTextView;
}

// UpdateProperty
void
FloatValueView::UpdateProperty()
{
	if (fProperty && fProperty->SetValue(fTextView->FloatValue()))
		ValueChanged(fProperty);
}

// ValueChanged
void
FloatValueView::ValueChanged(Property* property)
{
//	if (fProperty && fProperty->SetValue(fTextView->Value()))
	if (fProperty)
		fTextView->SetValue(fProperty->Value());
	TextInputValueView::ValueChanged(property);
}

// AdoptProperty
bool
FloatValueView::AdoptProperty(Property* property)
{
	FloatProperty* p = dynamic_cast<FloatProperty*>(property);
	if (p) {
		if (roundf(fTextView->FloatValue() * 100.0)
				!= roundf(p->Value() * 100.0)) {
			fTextView->SetValue(p->Value());
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
FloatValueView::GetProperty() const
{
	return fProperty;
}




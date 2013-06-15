/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FontProperty.h"

#include <new>
#include <stdio.h>

#include "support_ui.h"
#include "ui_defines.h"

// constructor
FontProperty::FontProperty(uint32 identifier)
	: Property(identifier),
	  fValue(be_plain_font)
{
}

// constructor
FontProperty::FontProperty(uint32 identifier, const Font& font)
	: Property(identifier),
	  fValue(font)
{
}

// constructor
FontProperty::FontProperty(const FontProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destrucor
FontProperty::~FontProperty()
{
}

// Clone
Property*
FontProperty::Clone(bool deep) const
{
	return new (std::nothrow) FontProperty(*this, deep);
}

// SetValue
bool
FontProperty::SetValue(const char* _string)
{
	BString string(_string);

	// find family part
	string.RemoveFirst("font-family:");
	int32 pos = string.FindFirst(';');

	BString family;
	family.SetTo(string, pos);

	pos = string.FindFirst("font-style:");
	string.Remove(0, pos + 11);

	// find style part
	pos = string.FindFirst(';');
	BString style;
	style.SetTo(string, pos);

	font_family fontFamily;
	font_style fontStyle;
	sprintf(fontFamily, "%s", family.String());
	sprintf(fontStyle, "%s", style.String());

	if (family.Length() == 0 || style.Length() == 0)
		return false;

	Font value(fValue);
	value.SetFamilyAndStyle(fontFamily, fontStyle);

	return SetValue(value);
}

// SetValue
bool
FontProperty::SetValue(const Property* other)
{
	const FontProperty* c = dynamic_cast<const FontProperty*>(other);
	if (c) {
		return SetValue(c->Value());
	}
	return false;
}

// GetValue
void
FontProperty::GetValue(BString& string)
{
	font_family family;
	font_style style;

	fValue.GetFamilyAndStyle(&family, &style);

	string << "font-family:" << family << "; ";
	string << "font-style:" << style << ";";
}

// Equals
bool
FontProperty::Equals(const Property* other) const
{
	const FontProperty* f = dynamic_cast<const FontProperty*>(other);
	if (f) {
		// NOTE: we don't simply compare fValue == f->fValue,
		// because the font property is supposed to look only
		// for the family/style
		font_family ourFamily;
		font_style ourStyle;
		fValue.GetFamilyAndStyle(&ourFamily, &ourStyle);

		font_family otherFamily;
		font_style otherStyle;
		f->fValue.GetFamilyAndStyle(&otherFamily, &otherStyle);

		return strcmp(ourFamily, otherFamily) == 0
			&& strcmp(ourStyle, otherStyle) == 0;
	}
	return false;
}

// MakeAnimatable
bool
FontProperty::MakeAnimatable(bool animatable)
{
	return false;
}

// #pragma mark -

// SetValue
bool
FontProperty::SetValue(const Font& value)
{
	if (fValue != value) {
		fValue = value;
		return true;
	}
	return false;
}

// Value
Font
FontProperty::Value() const
{
	return fValue;
}

// GetFontName
void
FontProperty::GetFontName(BString* string) const
{
	if (!string)
		return;

	font_family family;
	font_style style;
	fValue.GetFamilyAndStyle(&family, &style);
	string->SetTo("");
	*string << family << "-" << style;
}



/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "AttributeAccessor.h"


// constructor
AttributeAccessor::AttributeAccessor()
{
}


// destructor
AttributeAccessor::~AttributeAccessor()
{
}


// SetAttribute
status_t
AttributeAccessor::SetAttribute(const char* name, const BFont& font)
{
	return _SetFontAttribute(name, font);
}


// SetAttribute
status_t
AttributeAccessor::SetAttribute(const char* name, const Font& font)
{
	return _SetFontAttribute(name, font);
}


// GetAttribute
rgb_color
AttributeAccessor::GetAttribute(const char* name, rgb_color defaultValue)
{
	rgb_color value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK) {
		const char* str = valueString.String();
		if (*str == '#') {
			str++;
			int32 length = strlen(str);
			unsigned scannedColor = 0;
			if (length == 3) {
				// if there are only 3 byte, than it means that we
				// need to expand the color (#f60 -> #ff6600)
				// TODO: There must be an easier way...
				char expanded[7];
				expanded[0] = *str;
				expanded[1] = *str++;
				expanded[2] = *str;
				expanded[3] = *str++;
				expanded[4] = *str;
				expanded[5] = *str;
				expanded[6] = 0;
				sscanf(expanded, "%x", &scannedColor);
			} else {
				sscanf(str, "%x", &scannedColor);
			}
			uint8* colorByte = (uint8*)&scannedColor;
			value.red	= colorByte[3];
			value.green	= colorByte[2];
			value.blue	= colorByte[1];
			value.alpha	= colorByte[0];
		} else {
			// TODO: parse "named color"
		}
	}
	return value;
}


// GetAttribute
BFont
AttributeAccessor::GetAttribute(const char* name, const BFont& _defaultValue)
{
	Font defaultValue(_defaultValue);
	return GetAttribute(name, defaultValue).GetBFont();
}


// GetAttribute
Font
AttributeAccessor::GetAttribute(const char* name, const Font& defaultValue)
{
	Font value(defaultValue);
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK) {
		// find family part
		valueString.RemoveFirst("font-family:");
		int32 pos = valueString.FindFirst(';');

		BString family;
		family.SetTo(valueString, pos);

		pos = valueString.FindFirst("font-style:");
		valueString.Remove(0, pos + 11);

		// find style part
		pos = valueString.FindFirst(';');
		BString style;
		style.SetTo(valueString, pos);

		font_family fontFamily;
		font_style fontStyle;
		sprintf(fontFamily, "%s", family.String());
		sprintf(fontStyle, "%s", style.String());

		if (family.Length() == 0 || style.Length() == 0)
			return value;

		// find size part
		pos = valueString.FindFirst("font-size:");
		valueString.Remove(0, pos + 10);

		pos = valueString.FindFirst(';');
		BString size;
		size.SetTo(valueString, pos);
		float fontSize = atof(size.String());

		value.SetFamilyAndStyle(fontFamily, fontStyle);
		if (fontSize > 0.0)
			value.SetSize(fontSize);
	}
	return value;
}


// HasAttribute
bool
AttributeAccessor::HasAttribute(const char* name)
{
	BString value;
	return (name && GetAttribute(name, value) == B_OK);
}


// _SetAttribute
template<class FontClass>
status_t
AttributeAccessor::_SetFontAttribute(const char* name, const FontClass& font)
{
	font_family family;
	font_style style;

	font.GetFamilyAndStyle(&family, &style);

	BString valueString;
	valueString << "font-family:" << family << "; ";
	valueString << "font-style:" << style << ";";
	valueString << "font-size:" << font.Size() << ";";

	return SetAttribute(name, valueString.String());
}

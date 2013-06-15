/*
 * Copyright 2008-2009 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 1998 Eric Shepherd.
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */

//! Be Newsletter Volume II, Issue 35; September 2, 1998 (Eric Shepherd)

#include "AttributeMessage.h"

#include <stdio.h>

#include <Entry.h>
#include <File.h>
#include <String.h>


AttributeMessage::AttributeMessage()
	:
	BMessage()
{
}


AttributeMessage::~AttributeMessage()
{
}


// #pragma mark - Setters


status_t
AttributeMessage::SetAttribute(const char* name, bool value)
{
	if (ReplaceBool(name, value) == B_OK)
		return B_OK;
	return AddBool(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, int8 value)
{
	if (ReplaceInt8(name, value) == B_OK)
		return B_OK;
	return AddInt8(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, uint8 value)
{
	if (ReplaceUInt8(name, value) == B_OK)
		return B_OK;
	return AddUInt8(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, int16 value)
{
	if (ReplaceInt16(name, value) == B_OK)
		return B_OK;
	return AddInt16(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, int32 value)
{
	if (ReplaceInt32(name, value) == B_OK)
		return B_OK;
	return AddInt32(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, uint32 value)
{
	if (ReplaceInt32(name, (int32)value) == B_OK)
		return B_OK;
	return AddInt32(name, (int32)value);
}


status_t
AttributeMessage::SetAttribute(const char* name, int64 value)
{
	if (ReplaceInt64(name, value) == B_OK)
		return B_OK;
	return AddInt64(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, uint64 value)
{
	if (ReplaceUInt64(name, value) == B_OK)
		return B_OK;
	return AddUInt64(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, float value)
{
	if (ReplaceFloat(name, value) == B_OK)
		return B_OK;
	return AddFloat(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, double value)
{
	if (ReplaceDouble(name, value) == B_OK)
		return B_OK;
	return AddDouble(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const char* value)
{
	if (ReplaceString(name, value) == B_OK)
		return B_OK;
	return AddString(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const rgb_color& c)
{
	uint32 value = (c.red << 24) | (c.green << 16) | (c.blue << 8) | c.alpha;
	if (ReplaceUInt32(name, value) == B_OK)
		return B_OK;
	return AddUInt32(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const BString& value)
{
	return SetAttribute(name, value.String());
}


status_t
AttributeMessage::SetAttribute(const char* name, const BPoint& value)
{
	if (ReplacePoint(name, value) == B_OK)
		return B_OK;
	return AddPoint(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const BRect& value)
{
	if (ReplaceRect(name, value) == B_OK)
		return B_OK;
	return AddRect(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const entry_ref& value)
{
	if (ReplaceRef(name, &value) == B_OK)
		return B_OK;
	return AddRef(name, &value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const BMessage* value)
{
	if (ReplaceMessage(name, value) == B_OK)
		return B_OK;
	return AddMessage(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const BFlattenable* value)
{
	if (ReplaceFlat(name, const_cast<BFlattenable*>(value)) == B_OK)
		return B_OK;
	return AddFlat(name, const_cast<BFlattenable*>(value));
}


status_t
AttributeMessage::SetAttribute(const char* name, const BFont& value)
{
	return _SetFontAttribute(name, value);
}


status_t
AttributeMessage::SetAttribute(const char* name, const Font& value)
{
	return _SetFontAttribute(name, value);
}


// #pragma mark - Getters

bool
AttributeMessage::GetAttribute(const char* name, bool defaultValue) const
{
	bool value;
	if (FindBool(name, &value) != B_OK)
		return defaultValue;
	return value;
}


int8
AttributeMessage::GetAttribute(const char* name, int8 defaultValue) const
{
	int8 value;
	if (FindInt8(name, &value) != B_OK)
		return defaultValue;
	return value;
}


uint8
AttributeMessage::GetAttribute(const char* name, uint8 defaultValue) const
{
	uint8 value;
	if (FindUInt8(name, &value) != B_OK)
		return defaultValue;
	return value;
}


int16
AttributeMessage::GetAttribute(const char* name, int16 defaultValue) const
{
	int16 value;
	if (FindInt16(name, &value) != B_OK)
		return defaultValue;
	return value;
}


int32
AttributeMessage::GetAttribute(const char* name, int32 defaultValue) const
{
	int32 value;
	if (FindInt32(name, &value) != B_OK)
		return defaultValue;
	return value;
}


uint32
AttributeMessage::GetAttribute(const char* name, uint32 defaultValue) const
{
	int32 value;
	if (FindInt32(name, &value) != B_OK)
		return defaultValue;
	return (uint32)value;
}


int64
AttributeMessage::GetAttribute(const char* name, int64 defaultValue) const
{
	int64 value;
	if (FindInt64(name, &value) != B_OK)
		return defaultValue;
	return value;
}


uint64
AttributeMessage::GetAttribute(const char* name, uint64 defaultValue) const
{
	uint64 value;
	if (FindUInt64(name, &value) != B_OK)
		return defaultValue;
	return value;
}


float
AttributeMessage::GetAttribute(const char* name, float defaultValue) const
{
	float value;
	if (FindFloat(name, &value) != B_OK)
		return defaultValue;
	return value;
}


double
AttributeMessage::GetAttribute(const char* name, double defaultValue) const
{
	double value;
	if (FindDouble(name, &value) != B_OK)
		return defaultValue;
	return value;
}


const char*
AttributeMessage::GetAttribute(const char* name,
	const char* defaultValue) const
{
	const char* value;
	if (FindString(name, &value) != B_OK)
		return defaultValue;
	return value;
}


rgb_color
AttributeMessage::GetAttribute(const char* name, rgb_color defaultValue) const
{
	uint32 value;
	if (FindUInt32(name, &value) != B_OK)
		return defaultValue;

	rgb_color color;
	color.red = (value & 0xff000000) >> 24;
	color.green = (value & 0x00ff0000) >> 16;
	color.blue = (value & 0x0000ff00) >> 8;
	color.alpha = value & 0x000000ff;
	return color;
}


BString
AttributeMessage::GetAttribute(const char* name,
	const BString& defaultValue) const
{
	BString value;
	if (FindString(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BPoint
AttributeMessage::GetAttribute(const char *name, BPoint defaultValue) const
{
	BPoint value;
	if (FindPoint(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BRect
AttributeMessage::GetAttribute(const char* name, BRect defaultValue) const
{
	BRect value;
	if (FindRect(name, &value) != B_OK)
		return defaultValue;
	return value;
}


entry_ref
AttributeMessage::GetAttribute(const char* name,
	const entry_ref& defaultValue) const
{
	entry_ref value;
	if (FindRef(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BMessage
AttributeMessage::GetAttribute(const char* name,
	const BMessage& defaultValue) const
{
	BMessage value;
	if (FindMessage(name, &value) != B_OK)
		return defaultValue;
	return value;
}


// GetAttribute
BFont
AttributeMessage::GetAttribute(const char* name,
	const BFont& _defaultValue) const
{
	Font defaultValue(_defaultValue);
	return GetAttribute(name, defaultValue).GetBFont();
}


// GetAttribute
Font
AttributeMessage::GetAttribute(const char* name,
	const Font& defaultValue) const
{
	Font value(defaultValue);
	BString valueString;
	BMessage archivedFont;
	if (FindMessage(name, &archivedFont) == B_OK) {
		// find family part
		const char* family;
		const char* style;
		float size;
		if (archivedFont.FindString("family", &family) == B_OK
			&& archivedFont.FindString("style", &style) == B_OK
			&& archivedFont.FindFloat("size", &size) == B_OK) {
			font_family fontFamily;
			font_style fontStyle;
			snprintf(fontFamily, sizeof(fontFamily), "%s", family);
			snprintf(fontStyle, sizeof(fontStyle), "%s", style);

			if (strlen(family) == 0 || strlen(style) == 0)
				return value;

			value.SetFamilyAndStyle(fontFamily, fontStyle);
			if (size > 0.0f)
				value.SetSize(size);
		}
	}
	return value;
}


// HasAttribute
bool
AttributeMessage::HasAttribute(const char* name) const
{
	type_code typeFound;
	int32 countFound;
	return GetInfo(name, &typeFound, &countFound) == B_OK && countFound > 0;
}


// #pragma mark -


template<class FontClass>
status_t
AttributeMessage::_SetFontAttribute(const char* name, const FontClass& font)
{
	font_family family;
	font_style style;

	font.GetFamilyAndStyle(&family, &style);

	BMessage archivedFont;
	status_t ret = archivedFont.AddString("family", family);
	if (ret == B_OK)
		ret = archivedFont.AddString("style", style);
	if (ret == B_OK)
		ret = archivedFont.AddFloat("size", font.Size());
	// TODO: Store more properties?

	if (ret != B_OK)
		return ret;

	if (ReplaceMessage(name, &archivedFont) == B_OK)
		return B_OK;
	return AddMessage(name, &archivedFont);
}


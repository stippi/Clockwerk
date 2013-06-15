/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ATTRIBUTE_ACCESSOR_H
#define ATTRIBUTE_ACCESSOR_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GraphicsDefs.h>
#include <String.h>

#include "Font.h"


class AttributeAccessor {
public:
								AttributeAccessor();
	virtual						~AttributeAccessor();

	virtual	status_t			SetAttribute(const char* name,
											 const char* value)			= 0;
	inline	status_t			SetAttribute(const char* name,
											 const BString& value);
	inline	status_t			SetAttribute(const char* name,
											 bool value);
	inline	status_t			SetAttribute(const char* name,
											 uint8 value);
	inline	status_t			SetAttribute(const char* name,
											 int8 value);
	inline	status_t			SetAttribute(const char* name,
											 uint32 value);
	inline	status_t			SetAttribute(const char* name,
											 int32 value);
	inline	status_t			SetAttribute(const char* name,
											 uint64 value);
	inline	status_t			SetAttribute(const char* name,
											 int64 value);
	inline	status_t			SetAttribute(const char* name,
											 float value);
	inline	status_t			SetAttribute(const char* name,
											 double value);
	inline	status_t			SetAttribute(const char* name,
											 rgb_color color);
			status_t			SetAttribute(const char* name,
											 const BFont& font);
			status_t			SetAttribute(const char* name,
											 const Font& font);

	virtual	status_t			GetAttribute(const char* name,
											 BString& value)			= 0;
	inline	BString				GetAttribute(const char* name,
											 const char* defaultValue);
	inline	bool				GetAttribute(const char* name,
											 bool defaultValue);
	inline	uint8				GetAttribute(const char* name,
											 uint8 defaultValue);
	inline	int8				GetAttribute(const char* name,
											 int8 defaultValue);
	inline	uint32				GetAttribute(const char* name,
											 uint32 defaultValue);
	inline	int32				GetAttribute(const char* name,
											 int32 defaultValue);
	inline	uint64				GetAttribute(const char* name,
											 uint64 defaultValue);
	inline	int64				GetAttribute(const char* name,
											 int64 defaultValue);
	inline	float				GetAttribute(const char* name,
											 float defaultValue);
	inline	double				GetAttribute(const char* name,
											 double defaultValue);
			rgb_color			GetAttribute(const char* name,
											 rgb_color defaultValue);
			BFont				GetAttribute(const char* name,
											 const BFont& defaultValue);
			Font				GetAttribute(const char* name,
											 const Font& defaultValue);

	virtual	bool				HasAttribute(const char* name);

 private:
			template<class FontClass>
			status_t			_SetFontAttribute(const char* name,
									const FontClass& font);
};



// inline members

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, const BString& value)
{
	return SetAttribute(name, value.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, bool value)
{
	return SetAttribute(name, value ? "true" : "false");
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, uint8 value)
{
	return SetAttribute(name, (uint32)value);
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, int8 value)
{
	return SetAttribute(name, (int32)value);
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, uint32 value)
{
	BString valueString;
	valueString << value;
	return SetAttribute(name, valueString.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, int32 value)
{
	BString valueString;
	valueString << value;
	return SetAttribute(name, valueString.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, uint64 value)
{
	BString valueString;
	valueString << value;
	return SetAttribute(name, valueString.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, int64 value)
{
	BString valueString;
	valueString << value;
	return SetAttribute(name, valueString.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, float value)
{
	BString valueString;
	// workarround for more digits after point (5 instead of 2)
	valueString << (float)(floorf(value * 100.0) / 100.0);
	int32 threeMoreDigits = (int32)((100.0 * value - floorf(100.0 * value)) * 1000.0);
	valueString << threeMoreDigits;
	return SetAttribute(name, valueString.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, double value)
{
	BString valueString;
	// unfortunately there's no double version of the << operator
	valueString << (float)value;
	return SetAttribute(name, valueString.String());
}

// SetAttribute
inline
status_t
AttributeAccessor::SetAttribute(const char* name, rgb_color c)
{
	char valueString[16];
	sprintf(valueString, "#%02x%02x%02x%02x", c.red, c.green, c.blue, c.alpha);
	return SetAttribute(name, valueString);
}

// GetAttribute
inline
BString
AttributeAccessor::GetAttribute(const char* name, const char* defaultValue)
{
	BString value;
	if (GetAttribute(name, value) != B_OK)
		value = defaultValue;
	return value;
}

// GetAttribute
inline
bool
AttributeAccessor::GetAttribute(const char* name, bool defaultValue)
{
	bool value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = (valueString.ICompare("true") == 0
				 || valueString.ICompare("on") == 0);
	return value;
}

// GetAttribute
inline
uint8
AttributeAccessor::GetAttribute(const char* name, uint8 defaultValue)
{
	uint32 value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = atol(valueString.String());
	return (uint8)max_c(0, min_c(255, value));
}

// GetAttribute
inline
int8
AttributeAccessor::GetAttribute(const char* name, int8 defaultValue)
{
	int32 value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = atol(valueString.String());
	return (int8)max_c(-128, min_c(127, value));
}

// GetAttribute
inline
uint32
AttributeAccessor::GetAttribute(const char* name, uint32 defaultValue)
{
	uint32 value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = atol(valueString.String());
	return value;
}

// GetAttribute
inline
int32
AttributeAccessor::GetAttribute(const char* name, int32 defaultValue)
{
	int32 value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = atol(valueString.String());
	return value;
}

// GetAttribute
inline
uint64
AttributeAccessor::GetAttribute(const char* name, uint64 defaultValue)
{
	uint64 value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
// PPC: no atoll
//		value = atoll(valueString.String());
		value = strtoll(valueString.String(), NULL, 10);
	return value;
}

// GetAttribute
inline
int64
AttributeAccessor::GetAttribute(const char* name, int64 defaultValue)
{
	int64 value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
// PPC: no atoll
//		value = atoll(valueString.String());
		value = strtoll(valueString.String(), NULL, 10);
	return value;
}

// GetAttribute
inline
float
AttributeAccessor::GetAttribute(const char* name, float defaultValue)
{
	float value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = (float)atof(valueString.String());
	return value;
}

// GetAttribute
inline
double
AttributeAccessor::GetAttribute(const char* name, double defaultValue)
{
	double value = defaultValue;
	BString valueString;
	if (GetAttribute(name, valueString) == B_OK)
		value = atof(valueString.String());
	return value;
}


#endif	// ATTRIBUTE_ACCESSOR_H

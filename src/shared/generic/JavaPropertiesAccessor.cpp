/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "JavaPropertiesAccessor.h"


// constructor
JavaPropertiesAccessor::JavaPropertiesAccessor(JavaProperties& properties)
	: fProperties(properties)
{
}


// destructor
JavaPropertiesAccessor::~JavaPropertiesAccessor()
{
}


// SetAttribute
status_t
JavaPropertiesAccessor::SetAttribute(const char* name, const char* value)
{
	return fProperties.SetProperty(name, value) ? B_OK : B_NO_MEMORY;
}


// GetAttribute
status_t
JavaPropertiesAccessor::GetAttribute(const char* name, BString& _value)
{
	const char* value = fProperties.GetProperty(name);
	if (!value)
		return B_ENTRY_NOT_FOUND;

	_value = value;
	if (_value.Length() != (int32)strlen(value))
		return B_NO_MEMORY;

	return B_OK;
}

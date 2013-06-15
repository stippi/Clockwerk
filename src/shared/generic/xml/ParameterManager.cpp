/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <Message.h>
#include <String.h>

#include "ParameterManager.h"

// constructor
ParameterManager::ParameterManager()
	: fParameters(NULL)
{
	fParameters = new BMessage;
}

// destructor
ParameterManager::~ParameterManager()
{
	delete fParameters;
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, const char* value)
{
	if (name && value) {
		RemoveParameter(name);
		fParameters->AddString(name, value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, bool value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddBool(name, value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, uint32 value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddInt32(name, (int32)value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, int32 value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddInt32(name, value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, uint64 value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddInt64(name, (int64)value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, int64 value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddInt64(name, value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, float value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddFloat(name, value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, double value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddDouble(name, value);
	}
}

// SetParameter
void
ParameterManager::SetParameter(const char* name, const void* value)
{
	if (name) {
		RemoveParameter(name);
		fParameters->AddPointer(name, value);
	}
}

// RemoveParameter
bool
ParameterManager::RemoveParameter(const char* name)
{
	return (name && fParameters->RemoveData(name) == B_OK);
}

// GetParameter
bool
ParameterManager::GetParameter(const char* name, BString& value) const
{
	return (name && fParameters->FindString(name, &value) == B_OK);
}

// GetParameter
BString
ParameterManager::GetParameter(const char* name, const char* defaultValue) const
{
	BString value;
	if (!name || fParameters->FindString(name, &value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
bool
ParameterManager::GetParameter(const char* name, bool defaultValue) const
{
	bool value;
	if (!name || fParameters->FindBool(name, &value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
uint32
ParameterManager::GetParameter(const char* name, uint32 defaultValue) const
{
	uint32 value;
	if (!name || fParameters->FindInt32(name, (int32)&value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
int32
ParameterManager::GetParameter(const char* name, int32 defaultValue) const
{
	int32 value;
	if (!name || fParameters->FindInt32(name, &value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
uint64
ParameterManager::GetParameter(const char* name, uint64 defaultValue) const
{
	uint64 value;
	if (!name || fParameters->FindInt64(name, (int64*)&value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
int64
ParameterManager::GetParameter(const char* name, int64 defaultValue) const
{
	int64 value;
	if (!name || fParameters->FindInt64(name, &value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
float
ParameterManager::GetParameter(const char* name, float defaultValue) const
{
	float value;
	if (!name || fParameters->FindFloat(name, &value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
double
ParameterManager::GetParameter(const char* name, double defaultValue) const
{
	double value;
	if (!name || fParameters->FindDouble(name, &value) != B_OK)
		value = defaultValue;
	return value;
}

// GetParameter
void*
ParameterManager::GetParameter(const char* name,
							   const void* defaultValue) const
{
	void* value;
	if (!name || fParameters->FindPointer(name, &value) != B_OK)
		value = const_cast<void*>(defaultValue);
	return value;
}

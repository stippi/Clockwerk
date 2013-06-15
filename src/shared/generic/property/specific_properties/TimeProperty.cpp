/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "TimeProperty.h"

#include <new>
#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "support.h"

// constructor
TimeProperty::TimeProperty(uint32 identifier, uint32 timeOfDayInSeconds)
	: Property(identifier),
	  fValue(timeOfDayInSeconds)
{
}

// constructor
TimeProperty::TimeProperty(const TimeProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
TimeProperty::~TimeProperty()
{
}

// Clone
Property*
TimeProperty::Clone(bool deep) const
{
	return new (std::nothrow) TimeProperty(*this, deep);
}

// SetValue
bool
TimeProperty::SetValue(const char* value)
{
	time_t now = time(NULL);
	time_t time = parsedate(value, now);
	tm t = *localtime(&time);

	return SetValue(t.tm_hour * 60 * 60 + t.tm_min * 60 + t.tm_sec);
}

// SetValue
bool
TimeProperty::SetValue(const Property* other)
{
	const TimeProperty* timeOther = dynamic_cast<const TimeProperty*>(other);
	if (timeOther) {
		return SetValue(timeOther->Value());
	}
	return false;
}

// GetValue
void
TimeProperty::GetValue(BString& string)
{
	string_for_time_of_day(string, fValue);
}

// Equals
bool
TimeProperty::Equals(const Property* other) const
{
	const TimeProperty* i = dynamic_cast<const TimeProperty*>(other);
	if (i) {
		return fValue == i->fValue;
	}
	return false;
}

// InterpolateTo
bool
TimeProperty::InterpolateTo(const Property* other, float scale)
{
	return false;
}

// SetValue
bool
TimeProperty::SetValue(uint32 value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DurationProperty.h"

#include <new>
#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "support.h"

// constructor
DurationProperty::DurationProperty(uint32 identifier, int64 durationInFrames)
	: Property(identifier),
	  fValue(durationInFrames)
{
}

// constructor
DurationProperty::DurationProperty(const DurationProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
DurationProperty::~DurationProperty()
{
}

// Clone
Property*
DurationProperty::Clone(bool deep) const
{
	return new (std::nothrow) DurationProperty(*this, deep);
}

// SetValue
bool
DurationProperty::SetValue(const char* value)
{
	int hours, minutes, seconds, frames;
	if (sscanf(value, "%d:%d:%d.%d",
		&hours, &minutes, &seconds, &frames) == 4) {
		// nothing to do
	} else if (sscanf(value, "%d:%d:%d",
		&hours, &minutes, &seconds) == 3) {
		frames = 0;
	} else if (sscanf(value, "%d:%d.%d",
		&minutes, &seconds, &frames) == 3) {
		hours = 0;
	} else if (sscanf(value, "%d:%d",
		&minutes, &seconds) == 2) {
		hours = 0;
		frames = 0;
	} else if (sscanf(value, "%d.%d",
		&seconds, &frames) == 2) {
		hours = 0;
		minutes = 0;
	} else if (sscanf(value, "%d",
		&seconds) == 1) {
		hours = 0;
		minutes = 0;
		frames = 0;
	} else
		return false;

	return SetValue(((int64)hours * 60 * 60 + minutes * 60 + seconds) * 25
		+ frames);
}

// SetValue
bool
DurationProperty::SetValue(const Property* other)
{
	const DurationProperty* durationOther
		= dynamic_cast<const DurationProperty*>(other);
	if (durationOther) {
		return SetValue(durationOther->Value());
	}
	return false;
}

// GetValue
void
DurationProperty::GetValue(BString& string)
{
	string_for_frame_of_day(string, fValue);
}

// Equals
bool
DurationProperty::Equals(const Property* other) const
{
	const DurationProperty* d = dynamic_cast<const DurationProperty*>(other);
	if (d) {
		return fValue == d->fValue;
	}
	return false;
}

// InterpolateTo
bool
DurationProperty::InterpolateTo(const Property* other, float scale)
{
	return false;
}

// SetValue
bool
DurationProperty::SetValue(int64 value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}


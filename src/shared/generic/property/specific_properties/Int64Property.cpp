/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Int64Property.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>

// constructor
Int64Property::Int64Property(uint32 identifier, int64 value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
Int64Property::Int64Property(const Int64Property& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
Int64Property::~Int64Property()
{
}

// Clone
Property*
Int64Property::Clone(bool deep) const
{
	return new (std::nothrow) Int64Property(*this, deep);
}

// SetValue
bool
Int64Property::SetValue(const char* value)
{
	// TODO: atoll is defined for __INTEL__ only
	return SetValue(atoll(value));
}

// SetValue
bool
Int64Property::SetValue(const Property* other)
{
	const Int64Property* intOther = dynamic_cast<const Int64Property*>(other);
	if (intOther) {
		return SetValue(intOther->Value());
	}
	return false;
}

// GetValue
void
Int64Property::GetValue(BString& string)
{
	string << fValue;
}

// Equals
bool
Int64Property::Equals(const Property* other) const
{
	const Int64Property* i = dynamic_cast<const Int64Property*>(other);
	if (i) {
		return fValue == i->fValue;
	}
	return false;
}

// InterpolateTo
bool
Int64Property::InterpolateTo(const Property* other, float scale)
{
	const Int64Property* intOther = dynamic_cast<const Int64Property*>(other);
	if (intOther) {
		return SetValue(fValue + (int64)((double)(intOther->Value()
												  - fValue) * scale + 0.5));
	}
	return false;
}

// SetValue
bool
Int64Property::SetValue(int64 value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}


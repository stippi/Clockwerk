/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "WeekDaysProperty.h"

#include <new>
#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// constructor
WeekDaysProperty::WeekDaysProperty(uint32 identifier, uint8 weekDays)
	: Property(identifier),
	  fValue(weekDays)
{
}

// constructor
WeekDaysProperty::WeekDaysProperty(const WeekDaysProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
WeekDaysProperty::~WeekDaysProperty()
{
}

// Clone
Property*
WeekDaysProperty::Clone(bool deep) const
{
	return new (std::nothrow) WeekDaysProperty(*this, deep);
}

// SetValue
bool
WeekDaysProperty::SetValue(const char* value)
{
	uint8 days = 0;
	if (!value)
		return SetValue(days);

	BString t(value);
	if (t.IFindFirst("mo") >= 0)
		days |= MONDAY;
	if (t.IFindFirst("tue") >= 0)
		days |= TUESDAY;
	if (t.IFindFirst("we") >= 0)
		days |= WEDNESDAY;
	if (t.IFindFirst("thu") >= 0)
		days |= THURSDAY;
	if (t.IFindFirst("fri") >= 0)
		days |= FRIDAY;
	if (t.IFindFirst("sat") >= 0)
		days |= SATURDAY;
	if (t.IFindFirst("su") >= 0)
		days |= SUNDAY;

	return SetValue(days);
}

// SetValue
bool
WeekDaysProperty::SetValue(const Property* other)
{
	const WeekDaysProperty* wdOther = dynamic_cast<const WeekDaysProperty*>(other);
	if (wdOther) {
		return SetValue(wdOther->Value());
	}
	return false;
}

// GetValue
void
WeekDaysProperty::GetValue(BString& string)
{
	bool commata = false;
	if (fValue & MONDAY) {
		string << "Mo";
		commata = true;
	}
	if (fValue & TUESDAY) {
		if (commata)
			string << ", ";
		string << "Tue";
		commata = true;
	}
	if (fValue & WEDNESDAY) {
		if (commata)
			string << ", ";
		string << "We";
		commata = true;
	}
	if (fValue & THURSDAY) {
		if (commata)
			string << ", ";
		string << "Thu";
		commata = true;
	}
	if (fValue & FRIDAY) {
		if (commata)
			string << ", ";
		string << "Fri";
		commata = true;
	}
	if (fValue & SATURDAY) {
		if (commata)
			string << ", ";
		string << "Sat";
		commata = true;
	}
	if (fValue & SUNDAY) {
		if (commata)
			string << ", ";
		string << "Su";
	}
}

// Equals
bool
WeekDaysProperty::Equals(const Property* other) const
{
	const WeekDaysProperty* w = dynamic_cast<const WeekDaysProperty*>(other);
	if (w) {
		return fValue == w->fValue;
	}
	return false;
}

// InterpolateTo
bool
WeekDaysProperty::InterpolateTo(const Property* other, float scale)
{
	return false;
}

// SetValue
bool
WeekDaysProperty::SetValue(uint8 value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}


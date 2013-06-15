/*
 * Copyright 2004-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Property.h"

#include <new>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Message.h>

#include "support.h"

#include "PropertyAnimator.h"

using std::nothrow;

// constructor
Property::Property(uint32 identifier)
	: BArchivable(),
	  fIdentifier(identifier),
	  fAnimator(NULL),
	  fEditable(true)
{
}

// constructor
Property::Property(const Property& other, bool deep)
	: BArchivable(),
	  fIdentifier(other.fIdentifier),
	  fAnimator(deep && other.fAnimator ?
	  			new (nothrow) PropertyAnimator(this, *other.fAnimator)
	  			: NULL),
	  fEditable(other.fEditable)
{
}

// constructor
Property::Property(BMessage* archive)
	: BArchivable(),
	  fIdentifier(0),
	  fAnimator(NULL),
	  fEditable(true)
	  	// TODO: make PropertyAnimator archivable also
{
	if (!archive)
		return;

	if (archive->FindInt32("id", (int32*)&fIdentifier) < B_OK)
		fIdentifier = 0;
	if (archive->FindBool("editable", &fEditable) < B_OK)
		fEditable = true;
}

// destructor
Property::~Property()
{
	delete fAnimator;
}

// SetScale
bool
Property::SetScale(float scale)
{
	// default implementation is
	// for properties without min/max range
	return false;
}

// Scale
float
Property::Scale() const
{
	// default implementation returns
	// "arbitrary" value, it will result
	// in the property keyframe/value to
	// be displayed in the middle of of
	// the edit area
	return 0.5;
}

// InterpolateTo
bool
Property::InterpolateTo(const Property* other, float scale)
{
	// some properties don't support this
	return false;
}

// MakeAnimatable
bool
Property::MakeAnimatable(bool animatable)
{
	if (animatable) {
		if (!fAnimator)
			fAnimator = new (nothrow) PropertyAnimator(this);
	} else {
		delete fAnimator;
		fAnimator = NULL;
	}
	return true;
}

// SetEditable
void
Property::SetEditable(bool editable)
{
	fEditable = editable;
}

// #pragma mark -

// constructor
IntProperty::IntProperty(uint32 identifier, int32 value,
						 int32 min, int32 max)
	: Property(identifier),
	  fValue(value),
	  fMin(min),
	  fMax(max)
{
}

// constructor
IntProperty::IntProperty(const IntProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue),
	  fMin(other.fMin),
	  fMax(other.fMax)
{
}

// destructor
IntProperty::~IntProperty()
{
}

// Clone
Property*
IntProperty::Clone(bool deep) const
{
	return new (nothrow) IntProperty(*this, deep);
}

// SetValue
bool
IntProperty::SetValue(const char* value)
{
	return SetValue(atoi(value));
}

// SetValue
bool
IntProperty::SetValue(const Property* other)
{
	const IntProperty* intOther = dynamic_cast<const IntProperty*>(other);
	if (intOther) {
		return SetValue(intOther->Value());
	}
	return false;
}

// GetValue
void
IntProperty::GetValue(BString& string)
{
	string << fValue;
}

// Equals
bool
IntProperty::Equals(const Property* other) const
{
	const IntProperty* i = dynamic_cast<const IntProperty*>(other);
	if (i) {
		return fValue == i->fValue;
	}
	return false;
}

// SetScale
bool
IntProperty::SetScale(float scale)
{
	if (fMin != -2147483647 && fMax != 2147483647
		&& fMin < fMax) {
		return SetValue(fMin + (int32)((fMax - fMin) * scale + 0.5));
	}
	return false;
}

// Scale
float
IntProperty::Scale() const
{
	if (fMin != -2147483647 && fMax != 2147483647
		&& fMin < fMax) {
		return (fValue - fMin) / (float)(fMax - fMin);
	}
	return 0.5;
}

// InterpolateTo
bool
IntProperty::InterpolateTo(const Property* other, float scale)
{
	const IntProperty* intOther = dynamic_cast<const IntProperty*>(other);
	if (intOther) {
		return SetValue(fValue + (int32)((float)(intOther->Value()
												 - fValue) * scale + 0.5));
	}
	return false;
}

// SetValue
bool
IntProperty::SetValue(int32 value)
{
	// truncate
	if (value < fMin)
		value = fMin;
	if (value > fMax)
		value = fMax;

	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
FloatProperty::FloatProperty(uint32 identifier, float value,
							 float min, float max)
	: Property(identifier),
	  fValue(value),
	  fMin(min),
	  fMax(max)
{
}

// constructor
FloatProperty::FloatProperty(const FloatProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue),
	  fMin(other.fMin),
	  fMax(other.fMax)
{
}

// destructor
FloatProperty::~FloatProperty()
{
}

// Clone
Property*
FloatProperty::Clone(bool deep) const
{
	return new (nothrow) FloatProperty(*this, deep);
}

// SetValue
bool
FloatProperty::SetValue(const char* value)
{
	return SetValue(atof(value));
}

// SetValue
bool
FloatProperty::SetValue(const Property* other)
{
	const FloatProperty* ftOther = dynamic_cast<const FloatProperty*>(other);
	if (ftOther) {
		return SetValue(ftOther->Value());
	}
	return false;
}

// GetValue
void
FloatProperty::GetValue(BString& string)
{
	append_float(string, fValue, 4);
}

// Equals
bool
FloatProperty::Equals(const Property* other) const
{
	const FloatProperty* f = dynamic_cast<const FloatProperty*>(other);
	if (f) {
		return fValue == f->fValue;
	}
	return false;
}

// SetScale
bool
FloatProperty::SetScale(float scale)
{
	float range = fMax - fMin;
	if (range > 0.0 && range < 1000.0) {
		return SetValue(fMin + range * scale);
	}
	return false;
}

// Scale
float
FloatProperty::Scale() const
{
	float range = fMax - fMin;
	if (range > 0.0 && range < 1000.0) {
		return (fValue - fMin) / range;
	}
	return 0.5;
}

// InterpolateTo
bool
FloatProperty::InterpolateTo(const Property* other, float scale)
{
	const FloatProperty* floatOther = dynamic_cast<const FloatProperty*>(other);
	if (floatOther) {
		return SetValue(fValue + (floatOther->Value() - fValue) * scale);
	}
	return false;
}

// SetValue
bool
FloatProperty::SetValue(float value)
{
	// truncate
	if (value < fMin)
		value = fMin;
	if (value > fMax)
		value = fMax;

	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
UInt8Property::UInt8Property(uint32 identifier, uint8 value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
UInt8Property::UInt8Property(const UInt8Property& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
UInt8Property::~UInt8Property()
{
}

// Clone
Property*
UInt8Property::Clone(bool deep) const
{
	return new (nothrow) UInt8Property(*this, deep);
}

// SetValue
bool
UInt8Property::SetValue(const char* value)
{
	return SetValue((uint8)max_c(0, min_c(255, atoi(value))));
}

// SetValue
bool
UInt8Property::SetValue(const Property* other)
{
	const UInt8Property* uiOther = dynamic_cast<const UInt8Property*>(other);
	if (uiOther) {
		return SetValue(uiOther->Value());
	}
	return false;
}

// GetValue
void
UInt8Property::GetValue(BString& string)
{
	string << fValue;
}

// Equals
bool
UInt8Property::Equals(const Property* other) const
{
	const UInt8Property* u = dynamic_cast<const UInt8Property*>(other);
	if (u) {
		return fValue == u->fValue;
	}
	return false;
}

// SetScale
bool
UInt8Property::SetScale(float scale)
{
	return SetValue((uint8)(255.0 * scale + 0.5));
}

// Scale
float
UInt8Property::Scale() const
{
	return fValue / 255.0;
}

// InterpolateTo
bool
UInt8Property::InterpolateTo(const Property* other, float scale)
{
	const UInt8Property* uint8Other = dynamic_cast<const UInt8Property*>(other);
	if (uint8Other) {
		return SetValue(fValue + (uint8)((float)(uint8Other->Value()
												 - fValue) * scale + 0.5));
	}
	return false;
}

// SetValue
bool
UInt8Property::SetValue(uint8 value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
BoolProperty::BoolProperty(uint32 identifier, bool value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
BoolProperty::BoolProperty(const BoolProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
BoolProperty::~BoolProperty()
{
}

// Clone
Property*
BoolProperty::Clone(bool deep) const
{
	return new (nothrow) BoolProperty(*this, deep);
}

// SetValue
bool
BoolProperty::SetValue(const char* value)
{
	bool v;
	if (strcasecmp(value, "true") == 0)
		v = true;
	else if (strcasecmp(value, "on") == 0)
		v = true;
	else
		v = (bool)atoi(value);

	return SetValue(v);
}

// SetValue
bool
BoolProperty::SetValue(const Property* other)
{
	const BoolProperty* bOther = dynamic_cast<const BoolProperty*>(other);
	if (bOther) {
		return SetValue(bOther->Value());
	}
	return false;
}

// GetValue
void
BoolProperty::GetValue(BString& string)
{
	if (fValue)
		string << "on";
	else
		string << "off";
}

// Equals
bool
BoolProperty::Equals(const Property* other) const
{
	const BoolProperty* b = dynamic_cast<const BoolProperty*>(other);
	if (b) {
		return fValue == b->fValue;
	}
	return false;
}

// InterpolateTo
bool
BoolProperty::InterpolateTo(const Property* other, float scale)
{
	const BoolProperty* boolOther = dynamic_cast<const BoolProperty*>(other);
	if (boolOther) {
		if (scale >= 0.5)
			return SetValue(boolOther->Value());
	}
	return false;
}

// SetValue
bool
BoolProperty::SetValue(bool value)
{
	if (value != fValue) {
		fValue = value;
		return true;
	}
	return false;
}

// #pragma mark -

// constructor
StringProperty::StringProperty(uint32 identifier, const char* value)
	: Property(identifier),
	  fValue(value)
{
}

// constructor
StringProperty::StringProperty(const StringProperty& other, bool deep)
	: Property(other, deep),
	  fValue(other.fValue)
{
}

// destructor
StringProperty::~StringProperty()
{
}

// Clone
Property*
StringProperty::Clone(bool deep) const
{
	return new (nothrow) StringProperty(*this);
}

// SetValue
bool
StringProperty::SetValue(const char* value)
{
	if (fValue != value) {
		fValue = value;
		return true;
	}
	return false;
}

// SetValue
bool
StringProperty::SetValue(const Property* other)
{
	const StringProperty* sOther = dynamic_cast<const StringProperty*>(other);
	if (sOther) {
		return SetValue(sOther->Value());
	}
	return false;
}

// GetValue
void
StringProperty::GetValue(BString& string)
{
	string << fValue;
}

// Equals
bool
StringProperty::Equals(const Property* other) const
{
	const StringProperty* s = dynamic_cast<const StringProperty*>(other);
	if (s) {
		return fValue == s->fValue;
	}
	return false;
}

// MakeAnimatable
bool
StringProperty::MakeAnimatable(bool)
{
	return false;
}

// SetValue
bool
StringProperty::SetValue(const BString& value)
{
	if (fValue != value) {
		fValue = value;
		return true;
	}
	return false;
}



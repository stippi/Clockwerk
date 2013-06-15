/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef DURATION_PROPERTY_H
#define DURATION_PROPERTY_H

#include "Property.h"

class DurationProperty : public Property {
 public:
								DurationProperty(uint32 identifier,
									int64 durationInFrames = 0);
								DurationProperty(const DurationProperty& other,
									bool deep);
	virtual						~DurationProperty();

	virtual	Property*			Clone(bool deep) const;

	virtual	type_code			Type() const
									{ return B_INT64_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				Equals(const Property* other) const;

	virtual	bool				InterpolateTo(const Property* other,
									float scale);

	// DurationProperty
			bool				SetValue(int64 value);

	inline	int64				Value() const
									{ return fValue; }

 private:
			int64				fValue;
};

#endif // DURATION_PROPERTY_H

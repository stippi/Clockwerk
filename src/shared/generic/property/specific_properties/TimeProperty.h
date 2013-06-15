/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TIME_PROPERTY_H
#define TIME_PROPERTY_H

#include "Property.h"

class TimeProperty : public Property {
 public:
								TimeProperty(uint32 identifier,
									uint32 timeOfDayInSeconds = 0);
								TimeProperty(const TimeProperty& other,
									bool deep);
	virtual						~TimeProperty();

	virtual	Property*			Clone(bool deep) const;

	virtual	type_code			Type() const
									{ return B_TIME_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				Equals(const Property* other) const;

	virtual	bool				InterpolateTo(const Property* other,
									float scale);

	// TimeProperty
			bool				SetValue(uint32 value);

	inline	uint32				Value() const
									{ return fValue; }

 private:
			uint32				fValue;
};

#endif // TIME_PROPERTY_H

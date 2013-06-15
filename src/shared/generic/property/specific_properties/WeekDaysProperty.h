/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef WEEK_DAYS_PROPERTY_H
#define WEEK_DAYS_PROPERTY_H

#include "Property.h"

class WeekDaysProperty : public Property {
 public:
	enum {
		MONDAY		= 0x01,
		TUESDAY		= 0x02,
		WEDNESDAY	= 0x04,
		THURSDAY	= 0x08,
		FRIDAY		= 0x10,
		SATURDAY	= 0x20,
		SUNDAY		= 0x40,

		WORK_DAYS	= MONDAY | TUESDAY | WEDNESDAY | THURSDAY | FRIDAY,
		WEEKEND		= SATURDAY | SUNDAY,
		ALL_WEEK	= WORK_DAYS | WEEKEND
	};

								WeekDaysProperty(uint32 identifier,
											 uint8 weekDays = ALL_WEEK);
								WeekDaysProperty(const WeekDaysProperty& other,
											  bool deep);
	virtual						~WeekDaysProperty();

	virtual	Property*			Clone(bool deep) const;

	virtual	type_code			Type() const
									{ return B_UINT8_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				Equals(const Property* other) const;

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// IntProperty
			bool				SetValue(uint8 value);

	inline	uint8				Value() const
									{ return fValue; }

 private:
			uint8				fValue;
};

#endif // WEEK_DAYS_PROPERTY_H

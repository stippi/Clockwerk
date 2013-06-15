/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef INT64_PROPERTY_H
#define INT64_PROPERTY_H

#include "Property.h"

class Int64Property : public Property {
 public:
								Int64Property(uint32 identifier,
											  int64 value = 0);
								Int64Property(const Int64Property& other,
											  bool deep);
	virtual						~Int64Property();

	virtual	Property*			Clone(bool deep) const;

	virtual	type_code			Type() const
									{ return B_INT64_TYPE; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				Equals(const Property* other) const;

	virtual	bool				InterpolateTo(const Property* other,
											  float scale);

	// IntProperty
			bool				SetValue(int64 value);

	inline	int64				Value() const
									{ return fValue; }

 private:
			int64				fValue;
};

#endif // PROPERTY_H

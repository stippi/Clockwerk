/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef FONT_PROPERTY_H
#define FONT_PROPERTY_H


#include "Font.h"
#include "Property.h"


class FontProperty : public Property {
 public:
								FontProperty(uint32 identifier);
								FontProperty(uint32 identifier,
											 const Font& font);
								FontProperty(const FontProperty& other,
											 bool deep);

	virtual						~FontProperty();

	// Property interface
	virtual	Property*			Clone(bool deep) const;

	virtual	type_code			Type() const
//									{ return B_FONT_TYPE; } // not defined on R5
									{ return 'FONt'; }

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				Equals(const Property* other) const;

	// animation
	virtual	bool				MakeAnimatable(bool animatable = true);

	// FontProperty
			bool				SetValue(const Font& font);
			Font				Value() const;
			void				GetFontName(BString* string) const;

 private:
			Font				fValue;
};

#endif // FONT_PROPERTY_H



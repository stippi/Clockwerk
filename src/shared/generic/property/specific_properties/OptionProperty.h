/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef OPTION_PROPERTY_H
#define OPTION_PROPERTY_H

#include <List.h>
#include <String.h>

#include "Property.h"

class OptionProperty : public Property {
 public:
								OptionProperty(uint32 identifier);
								OptionProperty(const OptionProperty& other,
											   bool deep);
								OptionProperty(BMessage* archive);
	virtual						~OptionProperty();

	// Property interface
	virtual	Property*			Clone(bool deep) const;

	virtual	type_code			Type() const;

	virtual	bool				SetValue(const char* value);
	virtual	bool				SetValue(const Property* other);
	virtual	void				GetValue(BString& string);

	virtual	bool				Equals(const Property* other) const;

	// animation
	virtual	bool				MakeAnimatable(bool animatable = true);


	// BArchivable interface
	virtual	status_t			Archive(BMessage* archive, bool deep = true) const;
	
	__attribute__ ((visibility ("default")))
	static	BArchivable*		Instantiate(BMessage* archive);

	// OptionProperty
			int32				Value() const;
			bool				SetValue(int32 id);
									// the common property interface

			void				AddOption(int32 id, const char* name);

			int32				CurrentOptionID() const;
			bool				SetCurrentOptionID(int32 id);
									// same as Value() and SetValue(),
									// kept for backwards compatibility

			bool				GetOption(int32 index, BString* string, int32* id) const;
			bool				GetCurrentOption(BString* string) const;

			bool				SetOptionAtOffset(int32 indexOffset);

 private:

	struct option {
		int32		id;
		BString		name;
	};

			BList				fOptions;
			int32				fCurrentOptionID;
};


#endif // OPTION_PROPERTY_H



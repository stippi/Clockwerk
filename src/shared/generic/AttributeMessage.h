/*
 * Copyright 2008-2009 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 1998 Eric Shepherd.
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */
#ifndef SETTINGS_MESSAGE_H
#define SETTINGS_MESSAGE_H

#include <Font.h>
#include <Message.h>
#include <String.h>

#include "Font.h"

class AttributeMessage : public BMessage {
public:
								AttributeMessage();
	virtual  					~AttributeMessage();

	// Setters
			status_t			SetAttribute(const char* name, bool value);
			status_t			SetAttribute(const char* name, int8 value);
			status_t			SetAttribute(const char* name, uint8 value);
			status_t			SetAttribute(const char* name, int16 value);
			status_t			SetAttribute(const char* name, int32 value);
			status_t			SetAttribute(const char* name, uint32 value);
			status_t			SetAttribute(const char* name, int64 value);
			status_t			SetAttribute(const char* name, uint64 value);
			status_t			SetAttribute(const char* name, float value);
			status_t			SetAttribute(const char* name, double value);
			status_t			SetAttribute(const char* name,
									const char* value);
			status_t			SetAttribute(const char* name,
									const rgb_color& value);
			status_t			SetAttribute(const char* name,
									const BString& value);
			status_t			SetAttribute(const char* name,
									const BPoint& value);
			status_t			SetAttribute(const char* name,
									const BRect& value);
			status_t			SetAttribute(const char* name,
									const entry_ref& value);
			status_t			SetAttribute(const char* name,
									const BMessage* value);
			status_t			SetAttribute(const char* name,
									const BFlattenable* value);
			status_t			SetAttribute(const char* name,
									const BFont& font);
			status_t			SetAttribute(const char* name,
									const Font& font);


	// Getters
			bool				GetAttribute(const char* name,
									bool defaultValue) const;
			int8				GetAttribute(const char* name,
									int8 defaultValue) const;
			uint8				GetAttribute(const char* name,
									uint8 defaultValue) const;
			int16				GetAttribute(const char* name,
									int16 defaultValue) const;
			int32				GetAttribute(const char* name,
									int32 defaultValue) const;
			uint32				GetAttribute(const char* name,
									uint32 defaultValue) const;
			int64				GetAttribute(const char* name,
									int64 defaultValue) const;
			uint64				GetAttribute(const char* name,
									uint64 defaultValue) const;
			float				GetAttribute(const char* name,
									float defaultValue) const;
			double				GetAttribute(const char* name,
									double defaultValue) const;
			const char*			GetAttribute(const char* name,
									const char* defaultValue) const;
			rgb_color			GetAttribute(const char* name,
									rgb_color defaultValue) const;
			BString				GetAttribute(const char* name,
									const BString& defaultValue) const;
			BPoint				GetAttribute(const char* name,
									BPoint defaultValue) const;
			BRect				GetAttribute(const char* name,
									BRect defaultValue) const;
			entry_ref			GetAttribute(const char* name,
									const entry_ref& defaultValue) const;
			BMessage			GetAttribute(const char* name,
									const BMessage& defaultValue) const;
			BFont				GetAttribute(const char* name,
									const BFont& defaultValue) const;
			Font				GetAttribute(const char* name,
									const Font& defaultValue) const;

	virtual	bool				HasAttribute(const char* name) const;

private:
			template<class FontClass>
			status_t			_SetFontAttribute(const char* name,
									const FontClass& font);
};

#endif  // SETTINGS_MESSAGE_H

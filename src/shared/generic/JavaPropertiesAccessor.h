/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef JAVA_PROPERTIES_ACCESSOR_H
#define JAVA_PROPERTIES_ACCESSOR_H

#include "AttributeAccessor.h"
#include "JavaProperties.h"


class JavaPropertiesAccessor : public AttributeAccessor {
public:
								JavaPropertiesAccessor(
									JavaProperties& properties);
	virtual						~JavaPropertiesAccessor();

	virtual	status_t			SetAttribute(const char* name,
									const char* value);
	virtual	status_t			GetAttribute(const char* name,
									BString& value);

private:
			JavaProperties&		fProperties;
};




#endif	// JAVA_PROPERTIES_ACCESSOR_H

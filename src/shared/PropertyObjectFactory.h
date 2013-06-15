/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PROPERTY_OBJECT_FACTORY_H
#define PROPERTY_OBJECT_FACTORY_H

#include <SupportDefs.h>

class PropertyObject;


class PropertyObjectFactory {
public:
	static	status_t			CreatePropertyObject(const char* type,
									PropertyObject** object);

	static	status_t			InitPropertyObject(const char* type,
									PropertyObject* object);


};

#endif // PROPERTY_OBJECT_FACTORY_H

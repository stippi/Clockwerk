/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_STORABLE_H
#define XML_STORABLE_H

#include <SupportDefs.h>

// for convenience
//#include "XMLHelper.h"

class XMLHelper;

class XMLStorable {
 public:
								XMLStorable();
	virtual						~XMLStorable();

	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const	= 0;
	virtual	status_t			XMLRestore(XMLHelper& xmlHelper)		= 0;

 private:

};

#endif	// XML_STORABLE_H

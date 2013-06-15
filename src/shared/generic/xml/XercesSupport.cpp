/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <stdio.h>
#include <SupportDefs.h>

#include <DOM_Document.hpp>
#include <DOM_Node.hpp>
#include <DOM_NamedNodeMap.hpp>
#include <PlatformUtils.hpp>
#include <XMLFormatter.hpp>
#include <XMLUniDefs.hpp>

#include "XercesSupport.h"

// debugging
#include "Debug.h"
//#define ldebug	debug
#define ldebug	nodebug

// init_xerces
status_t
init_xerces()
{
	status_t error = B_ERROR;
	try {
		XMLPlatformUtils::Initialize();
		error = B_OK;
	} catch (const XMLException& toCatch) {
		std::cerr << "Error during initialization! :\n"
			<< StrX(toCatch.getMessage()) << std::endl;
	}
	return error;
}

// uninit_xerces
void
uninit_xerces()
{
    XMLPlatformUtils::Terminate();
}


XMLFormatter& operator<< (XMLFormatter& strm, const DOMString& s)
{
    unsigned int lent = s.length();

	if (lent <= 0)
		return strm;

    XMLCh*  buf = new XMLCh[lent + 1];
    XMLString::copyNString(buf, s.rawBuffer(), lent);
    buf[lent] = 0;
    strm << buf;
    delete [] buf;
    return strm;
}


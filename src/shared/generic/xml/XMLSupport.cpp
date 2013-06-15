/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "XMLSupport.h"

#include <stdio.h>

#include <SupportDefs.h>

#include "XercesDOMHelper.h"
#include "XercesSupport.h"

// debugging
#include "Debug.h"
//#define ldebug	debug
#define ldebug	nodebug

// init_xml
//
// Initializes the xml subsystem. Usually called in main, before running
// the application.
// Returns a value != B_OK, if an error occured.
status_t
init_xml()
{
	return init_xerces();
}

// uninit_xml
//
// Uninitializes the xml subsystem. Usually called in main, after
// the application terminated.
void
uninit_xml()
{
	return uninit_xerces();
}

// create_xml_helper
//
// Creates and returns a new XMLHelper. The caller takes over the ownership
// of the object and is therefore responsible for deleting it.
XMLHelper*
create_xml_helper()
{
	return new XercesDOMHelper;
}


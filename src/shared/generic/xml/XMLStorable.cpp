/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "XMLStorable.h"

// constructor
XMLStorable::XMLStorable()
{
}

// destructor
XMLStorable::~XMLStorable()
{
}

// XMLStore
//
// Stores the object to the supplied XMLHelper. Returns an error code, if
// something went wrong, B_OK otherwise.
// To be implemented by derived classes.
//status_t
//XMLStorable::XMLStore(XMLHelper& xmlHelper) const
//{
//	return B_ERROR;
//}

// XMLRestore
//
// Restores the object from the supplied XMLHelper. Returns an error code, if
// something went wrong, B_OK otherwise.
// To be implemented by derived classes.
//status_t
//XMLStorable::XMLRestore(XMLHelper& xmlHelper)
//{
//	return B_ERROR;
//}



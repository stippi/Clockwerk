/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RequestHandler.h"

// constructor
RequestHandler::RequestHandler()
{
}

// destructor
RequestHandler::~RequestHandler()
{
}

// HandleMessage
status_t
RequestHandler::HandleRequest(BMessage* request, RequestChannel* channel)
{
	return B_BAD_VALUE;
}

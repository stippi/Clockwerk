/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REQUEST_HANDLER_H
#define REQUEST_HANDLER_H

#include <SupportDefs.h>

#include "Compatibility.h"

class BMessage;
class RequestChannel;

class RequestHandler {
public:
								RequestHandler();
	virtual						~RequestHandler();

	virtual	status_t			HandleRequest(BMessage* request,
									RequestChannel* channel);
};

#endif	// REQUEST_HANDLER_H

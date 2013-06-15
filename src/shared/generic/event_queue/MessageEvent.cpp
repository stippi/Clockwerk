/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Message.h>

#include "MessageEvent.h"

// constructor
MessageEvent::MessageEvent(bigtime_t time, BHandler* handler, uint32 command)
	: Event(time),
	  AbstractLOAdapter(handler),
	  fCommand(command)
{
}

// constructor
MessageEvent::MessageEvent(bigtime_t time, const BMessenger& messenger)
	: Event(time),
	  AbstractLOAdapter(messenger)
{
}

// destructor
MessageEvent::~MessageEvent()
{
}

// Execute
void
MessageEvent::Execute()
{
	BMessage msg(fCommand);
	msg.AddInt64("time", Time());
	DeliverMessage(msg);
}


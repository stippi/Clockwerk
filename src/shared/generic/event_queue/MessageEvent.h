/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MESSAGE_EVENT_H
#define MESSAGE_EVENT_H

#include "AbstractLOAdapter.h"
#include "Event.h"

enum {
	MSG_EVENT	= 'evnt',
};

class MessageEvent : public Event, public AbstractLOAdapter {
 public:
								MessageEvent(bigtime_t time,
											 BHandler* handler,
											 uint32 command = MSG_EVENT);
								MessageEvent(bigtime_t time,
											 const BMessenger& messenger);
	virtual						~MessageEvent();

	virtual	void				Execute();

 private:
			uint32				fCommand;
};

#endif	// MESSAGE_EVENT_H

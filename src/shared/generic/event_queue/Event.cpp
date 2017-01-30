/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include "Event.h"

// constructor
Event::Event(bool autoDelete)
	: fTime(0),
	  fAutoDelete(autoDelete)
{
}

// constructor
Event::Event(bigtime_t time, bool autoDelete)
	: fTime(time),
	  fAutoDelete(autoDelete)
{
}

// destructor
Event::~Event()
{
}

// SetTime
void
Event::SetTime(bigtime_t time)
{
	fTime = time;
}

// Time
bigtime_t
Event::Time() const
{
	return fTime;
}

// SetAutoDelete
void
Event::SetAutoDelete(bool autoDelete)
{
	fAutoDelete = autoDelete;
}

// Execute
void
Event::Execute()
{
	printf("Event::Execute() - %" B_PRIdBIGTIME "\n", fTime);
}


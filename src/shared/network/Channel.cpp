/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Channel.h"


// constructor
Channel::Channel()
{
}


// destructor
Channel::~Channel()
{
}


// SendError
status_t
Channel::SendError(status_t error)
{
	Close();
	return error;
}


// ReceiveError
status_t
Channel::ReceiveError(status_t error)
{
	Close();
	return error;
}

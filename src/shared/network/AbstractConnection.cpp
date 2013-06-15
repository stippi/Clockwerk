/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "AbstractConnection.h"
#include "AutoLocker.h"
#include "Channel.h"
#include "Debug.h"

// constructor
AbstractConnection::AbstractConnection()
	: Connection(),
	  fInitStatus(B_NO_INIT),
	  fDownStreamChannel(NULL),
	  fUpStreamChannel(NULL),
	  fProtocolVersion(1)
{
}

// destructor
AbstractConnection::~AbstractConnection()
{
	// Danger: If derived classes implement Close(), this call will not reach
	// them. So, the caller should rather call Close() explicitely before
	// destroying the connection.
	Close();

	// delete the channels
	delete fDownStreamChannel;
	delete fUpStreamChannel;
}

// Init
status_t
AbstractConnection::Init()
{
	fInitStatus = B_OK;
	return fInitStatus;
}

// Close
void
AbstractConnection::Close()
{
	// close all channels
	if (fDownStreamChannel)
		fDownStreamChannel->Close();
	if (fUpStreamChannel)
		fUpStreamChannel->Close();
}

//// GetAuthorizedUser
//User*
//AbstractConnection::GetAuthorizedUser()
//{
//	return NULL;
//}

// SetDownStreamChannel
void
AbstractConnection::SetDownStreamChannel(Channel* channel)
{
	fDownStreamChannel = channel;
}

// SetUpStreamChannel
void
AbstractConnection::SetUpStreamChannel(Channel* channel)
{
	fUpStreamChannel = channel;
}

// DownStreamChannel
Channel*
AbstractConnection::DownStreamChannel() const
{
	return fDownStreamChannel;
}

// UpStreamChannel
Channel*
AbstractConnection::UpStreamChannel() const
{
	return fUpStreamChannel;
}

// ProtocolVersion
int32
AbstractConnection::ProtocolVersion() const
{
	return fProtocolVersion;
}

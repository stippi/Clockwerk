/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RequestConnection.h"

#include <stdio.h>

#include <new>

#include <Message.h>
#include <OS.h>

#include "AutoLocker.h"
#include "BlockingQueue.h"
#include "Channel.h"
#include "Connection.h"
#include "RequestChannel.h"
#include "RequestHandler.h"
#include "RequestMessageCodes.h"


// constructor
RequestConnection::RequestConnection(Connection* connection,
	RequestHandler* requestHandler, bool ownsRequestHandler)
	: fConnection(connection),
	  fRequestHandler(requestHandler),
	  fOwnsRequestHandler(ownsRequestHandler),
	  fUpStreamLock("up stream lock"),
	  fTerminationCount(0)
{
}


// destructor
RequestConnection::~RequestConnection()
{
	Close();
	delete fConnection;
	if (fOwnsRequestHandler)
		delete fRequestHandler;
}

// Init
status_t
RequestConnection::Init()
{
	// check parameters
	if (!fConnection || !fRequestHandler)
		return B_BAD_VALUE;
	if (!fConnection->DownStreamChannel() || !fConnection->UpStreamChannel())
		return B_ERROR;

	// create upstream channel
	fUpStreamChannel = new(std::nothrow) RequestChannel(
		fConnection->UpStreamChannel());
	if (!fUpStreamChannel)
		return B_NO_MEMORY;

	// check upstream lock
	if (fUpStreamLock.Sem() < 0)
		return B_NO_INIT;

	return B_OK;
}

// Close
void
RequestConnection::Close()
{
	atomic_add(&fTerminationCount, 1);
	if (fConnection)
		fConnection->Close();
}


// ProtocolVersion
int32
RequestConnection::ProtocolVersion() const
{
	return (fConnection ? fConnection->ProtocolVersion() : 0);
}


// SendRawData
status_t
RequestConnection::SendRawData(const void* buffer, int32 size)
{
	if (!buffer || size < 0)
		return B_BAD_VALUE;

	// get the upstream channel
	AutoLocker<BLocker> upstreamLocker(fUpStreamLock);

	if (fUpStreamChannel->IsClosed())
		return B_BUSTED_PIPE;

	// send the message
	status_t error = fUpStreamChannel->SendRawData(buffer, size);

	if (fUpStreamChannel->IsClosed())
		_Closed(error);

	return error;
}

// ReceiveRawData
status_t
RequestConnection::ReceiveRawData(void* buffer, int32 size)
{
	if (!buffer || size < 0)
		return B_BAD_VALUE;

	// get the upstream channel
	AutoLocker<BLocker> upstreamLocker(fUpStreamLock);

	if (fUpStreamChannel->IsClosed())
		return B_BUSTED_PIPE;

	// send the message
	status_t error = fUpStreamChannel->ReceiveRawData(buffer, size);

	if (fUpStreamChannel->IsClosed())
		_Closed(error);

	return error;
}

// SendString
status_t
RequestConnection::SendString(const char* text)
{
	if (text)
		return B_BAD_VALUE;

	// get the upstream channel
	AutoLocker<BLocker> upstreamLocker(fUpStreamLock);

	if (fUpStreamChannel->IsClosed())
		return B_BUSTED_PIPE;

	// send the message
	status_t error = fUpStreamChannel->SendString(text);

	if (fUpStreamChannel->IsClosed())
		_Closed(error);

	return error;
}

// SendRequest
status_t
RequestConnection::SendRequest(BMessage* message, BMessage* reply)
{
	return _SendRequest(message, reply, NULL);
}

// SendRequest
status_t
RequestConnection::SendRequest(BMessage* message, RequestHandler* replyHandler)
{
	if (!replyHandler)
		return B_BAD_VALUE;
	return _SendRequest(message, NULL, replyHandler);
}


// ReceiveRequest
status_t
RequestConnection::ReceiveRequest(BMessage* message)
{
	if (!message)
		return B_BAD_VALUE;

	// get the upstream channel
	AutoLocker<BLocker> upstreamLocker(fUpStreamLock);

	if (fUpStreamChannel->IsClosed())
		return B_BUSTED_PIPE;

	// send the message
	status_t error = fUpStreamChannel->ReceiveRequest(message);

	if (fUpStreamChannel->IsClosed())
		_Closed(error);

	return error;
}


// _SendRequest
status_t
RequestConnection::_SendRequest(BMessage* message, BMessage* reply,
	RequestHandler* replyHandler)
{
	// check parameters
	if (!message)
		return B_BAD_VALUE;

	AutoLocker<BLocker> upstreamLocker(fUpStreamLock);

	if (fUpStreamChannel->IsClosed())
		return B_BUSTED_PIPE;

	// send the message
	status_t error = fUpStreamChannel->SendRequest(message);

	// receive the reply
	if (error == B_OK && (reply || replyHandler)) {
		BMessage stackReply;
		if (!reply)
			reply = &stackReply;

		error = fUpStreamChannel->ReceiveRequest(reply);

		// let the handler (if any) handle the reply
		if (error == B_OK && replyHandler)
			error = replyHandler->HandleRequest(reply, fUpStreamChannel);
	}

	if (fUpStreamChannel->IsClosed())
		_Closed(error);

	return error;
}


// _Closed
void
RequestConnection::_Closed(status_t error)
{
	if (atomic_add(&fTerminationCount, 1) == 0 && fRequestHandler) {
		BMessage message(REQUEST_CONNECTION_BROKEN);
		message.AddInt32("error", error != B_OK ? error : B_ERROR);
		fRequestHandler->HandleRequest(&message, fUpStreamChannel);
	}
}

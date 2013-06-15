/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <new>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <ByteOrder.h>

#include "Channel.h"
#include "Compatibility.h"
#include "Debug.h"
#include "SocketConnection.h"
#include "svn_revision.h"
#include "NetAddress.h"
#include "NetFSDefs.h"

namespace SocketConnectionDefs {

const int32 kProtocolVersion = 3;

} // namespace SocketConnectionDefs

using namespace SocketConnectionDefs;


// #pragma mark -
// #pragma mark ----- SocketConnection -----

// constructor
SocketConnection::SocketConnection()
{
}

// destructor
SocketConnection::~SocketConnection()
{
}

// Init (client side)
status_t
SocketConnection::Init(const char* clientID, const char* parameters)
{
PRINT(("SocketConnection::Init\n"));
	if (!clientID || !parameters)
		return B_BAD_VALUE;
	status_t error = AbstractConnection::Init();
	if (error != B_OK)
		return error;

	fProtocolVersion = kProtocolVersion;

	// parse the parameters to get a server name and a port we shall connect to
	// parameter format is "<server>[:port]"
	char server[256];
	uint16 port = kDefaultSocketConnectionPort;
	if (strchr(parameters, ':')) {
		int result = sscanf(parameters, "%255[^:]:%hu", server, &port);
		if (result < 2)
			return B_BAD_VALUE;
	} else {
		int result = sscanf(parameters, "%255[^:]", server);
		if (result < 1)
			return B_BAD_VALUE;
	}

	// resolve server address
	NetAddress netAddress;
	error = NetAddressResolver().GetHostAddress(server, &netAddress);
	if (error != B_OK)
		return error;
	in_addr serverAddr = netAddress.GetAddress().sin_addr;

	// open the initial channel
	Channel* channel;
	error = OpenClientChannel(serverAddr, port, &channel);
	if (error != B_OK)
		return error;
	SetUpStreamChannel(channel);

	// send the server a connect request
	ConnectRequest request;
	request.protocolVersion = B_HOST_TO_BENDIAN_INT32(fProtocolVersion);
	request.serverAddress = serverAddr.s_addr;
	strcpy(request.clientID, clientID); 
	error = channel->Send(&request, sizeof(request));
	if (error != B_OK)
		return error;

	// get the server reply
	ConnectReply reply;
	size_t connectReplySize = (fProtocolVersion == 1
		? sizeof(ConnectReplyVersion1)
		: sizeof(ConnectReply));
PRINT(("  receiving server reply (%lu bytes)...\n", connectReplySize));
	error = channel->Receive(&reply, connectReplySize);
	if (error != B_OK)
		return error;
	error = B_BENDIAN_TO_HOST_INT32(reply.error);
	if (error != B_OK)
		return error;
	port = B_BENDIAN_TO_HOST_INT16(reply.port);
	if (fProtocolVersion >= 2)
		fProtocolVersion = B_BENDIAN_TO_HOST_INT32(reply.protocolVersion);

	// send a ackknowledge reply
	if (fProtocolVersion >= 2) {
		ConnectAckReply ackReply;
		ackReply.clientVersion = B_HOST_TO_BENDIAN_INT32(kSVNRevision);
		error = channel->Send(&ackReply, sizeof(ackReply));
		if (error != B_OK)
			return error;
	}

	// open the other channel
PRINT(("  creating other channel\n"));
	error = OpenClientChannel(serverAddr, port, &channel);
	if (error != B_OK)
		RETURN_ERROR(error);
	SetDownStreamChannel(channel);

	return B_OK;
}

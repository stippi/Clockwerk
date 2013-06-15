/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_SOCKET_CONNECTION_H
#define NET_FS_SOCKET_CONNECTION_H

#if BEOS_NETSERVER
#	include <socket.h>
#else
#	include <netinet/in.h>
#	include <sys/socket.h>
#endif

#include "AbstractConnection.h"

// SocketConnection
class SocketConnection : public AbstractConnection {
public:
								SocketConnection();
	virtual						~SocketConnection();

	virtual	status_t			Init(const char* clientID,
									const char* parameters);	// client side

protected:
	virtual	status_t			OpenClientChannel(in_addr serverAddr,
									uint16 port, Channel** channel) = 0;
};

// SocketConnectionDefs
namespace SocketConnectionDefs {

	// ConnectRequest
	struct ConnectRequest {
		int32	protocolVersion;
		uint32	serverAddress;
		char	clientID[256];
	} _PACKED;
	
	// ConnectReplyVersion1
	struct ConnectReplyVersion1 {
		int32	error;
		uint16	port;
	} _PACKED;

	// ConnectReply
	struct ConnectReply {
		int32	error;
		uint16	port;
		int32	protocolVersion;
		int32	serverVersion;
		char	serverID[256];
	} _PACKED;

	// ConnectAckReply
	struct ConnectAckReply {
		int32	clientVersion;
	} _PACKED;
	

	extern const int32 kProtocolVersion;

} // namespace SocketConnectionDefs

#endif	// NET_FS_SOCKET_CONNECTION_H

/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InsecureChannel.h"

#include <errno.h>

#ifdef BEOS_NETSERVER
#	include <socket.h>
#else
#	include <unistd.h>
#	include <netinet/in.h>
#	include <sys/socket.h>
#	include <sys/time.h>
#endif

#include "Compatibility.h"
#include "Debug.h"
#include "NetAddress.h"
#include "NetUtils.h"


static const int32	kMaxInterruptedTxCount	= 20;
	// number of subsequent "interrupted system calls" before giving up
	// sending/receiving
static const int32	kTxTimeout				= 30;
	// maximal number of seconds a send()/recv() is allowed to take


// constructor
InsecureChannel::InsecureChannel(int socket)
	: Channel(),
	  fSocket(socket),
	  fClosed(false)
{
	// set socket to blocking (with BONE a connection socket seems to inherit
	// the non-blocking property of the listener socket)
	if (fSocket >= 0) {
		int dontBlock = 0;
		setsockopt(fSocket, SOL_SOCKET, SO_NONBLOCK, &dontBlock, sizeof(int));

#if !BEOS_NETSERVER
		// send()/recv() timeout
		timeval timeout;
		timeout.tv_sec = kTxTimeout;
		timeout.tv_usec = 0;
		setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
		setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

		// low water mark
		int txLowWater = 1;
		setsockopt(fSocket, SOL_SOCKET, SO_SNDLOWAT, &txLowWater, sizeof(int));
		setsockopt(fSocket, SOL_SOCKET, SO_RCVLOWAT, &txLowWater, sizeof(int));

		// keep alive
		int keepAlive = 1;
		setsockopt(fSocket, SOL_SOCKET, SO_KEEPALIVE, &keepAlive, sizeof(int));
#endif
	}
}

// destructor
InsecureChannel::~InsecureChannel()
{
	Close();
}

// Close
void
InsecureChannel::Close()
{
	if (!fClosed) {
		fClosed = true;
		safe_closesocket(fSocket);
	}
}


// IsClosed
bool
InsecureChannel::IsClosed() const
{
	return fClosed;
}


// Send
status_t
InsecureChannel::Send(const void* _buffer, int32 size)
{
	if (size == 0)
		return B_OK;
	if (!_buffer || size < 0)
		return B_BAD_VALUE;

	const uint8* buffer = static_cast<const uint8*>(_buffer);
	int32 remainingRetries = kMaxInterruptedTxCount;

	while (size > 0) {
		int32 bytesSent = send(fSocket, buffer, size, 0);
		if (bytesSent < 0) {
			status_t error = errno;
			if (fClosed || errno != B_INTERRUPTED || --remainingRetries < 0)
				return SendError(error);
//PRINT(("InsecureChannel::Send: B_INTERRUPTED ignored, remaining retries: %ld\n",
//remainingRetries));
			bytesSent = 0;
		} else if (bytesSent == 0) {
			// This seems to indicate that the remote peer closed the
			// connection.
			PRINT(("Connection::Send(): sent 0 bytes. Assuming "
				"that the remote peer closed the connection\n"));
			return SendError(B_ERROR);
		} else {
			size -= bytesSent;
			buffer += bytesSent;
			remainingRetries = kMaxInterruptedTxCount;
//PRINT(("InsecureChannel::Send(): sent %ld bytes, still to sent: %lu\n",
//bytesSent, size));
		}
	}

	return B_OK;
}

// Receive
status_t
InsecureChannel::Receive(void* _buffer, int32 size)
{
	if (size == 0)
		return B_OK;
	if (!_buffer || size < 0)
		return B_BAD_VALUE;

	uint8* buffer = static_cast<uint8*>(_buffer);
	int32 remainingRetries = kMaxInterruptedTxCount;

	while (size > 0) {
		int32 bytesRead = recv(fSocket, buffer, size, 0);
		if (bytesRead < 0) {
			status_t error = errno;
			if (fClosed || error != B_INTERRUPTED || --remainingRetries < 0)
				return ReceiveError(error);
//PRINT(("InsecureChannel::Receive(): B_INTERRUPTED ignored\n"));
		} else if (bytesRead == 0) {
			// This seems to indicate that the remote peer closed the
			// connection.
			PRINT(("Connection::Receive(): received 0 bytes. Assuming "
				"that the remote peer closed the connection\n"));
			return ReceiveError(B_ERROR);
		} else {
			size -= bytesRead;
			buffer += bytesRead;
			remainingRetries = kMaxInterruptedTxCount;
//PRINT(("InsecureChannel::Receive(): received %ld bytes, still to read: %lu\n",
//bytesRead, size));
		}
	}

	return B_OK;
}

// GetPeerAddress
status_t
InsecureChannel::GetPeerAddress(NetAddress *address) const
{
	if (!address)
		return B_BAD_VALUE;

	sockaddr_in addr;
#ifdef __HAIKU__
	socklen_t
#else
	int 
#endif
	size = sizeof(sockaddr_in);
	if (getpeername(fSocket, (sockaddr*)&addr, &size) < 0)
		return errno;
	if (addr.sin_family != AF_INET)
		return B_BAD_VALUE;

	address->SetAddress(addr);
	return B_OK;
}


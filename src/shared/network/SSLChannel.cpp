/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <errno.h>

#include "Debug.h"
#include "SSLChannel.h"
//#include "NetAddress.h"

static const int32 kMaxInterruptedTxCount	= 20;
static const int32 kTxTimeout				= 30;

// constructor
SSLChannel::SSLChannel(CRYPT_SESSION session)
	: Channel(),
	  fSession(session),
	  fClosed(0)
{
	// set session to blocking
	cryptSetAttribute(fSession, CRYPT_OPTION_NET_READTIMEOUT, kTxTimeout);
	cryptSetAttribute(fSession, CRYPT_OPTION_NET_WRITETIMEOUT, kTxTimeout);
}

// destructor
SSLChannel::~SSLChannel()
{
	Close();
}

// Close
void
SSLChannel::Close()
{
	if (atomic_or(&fClosed, 1) == 0)
		cryptDestroySession(fSession);
}


// IsClosed
bool
SSLChannel::IsClosed() const
{
	return (atomic_or((vint32*)&fClosed, 0) != 0);
}


// Send
status_t
SSLChannel::Send(const void* _buffer, int32 size)
{
	if (size == 0)
		return B_OK;
	if (!_buffer || size < 0)
		return B_BAD_VALUE;

	const uint8* buffer = static_cast<const uint8*>(_buffer);
//	int32 remainingRetries = kMaxInterruptedTxCount;

	while (size > 0) {
		int bytesSent;
		int cryptError = cryptPushData(fSession, buffer, size, &bytesSent);
		if (cryptError != CRYPT_OK) {
			if (cryptError == CRYPT_ERROR_TIMEOUT)
				return SendError(B_TIMED_OUT);
			return SendError(B_ERROR);
		}

		if (bytesSent == 0) {
// TODO: Verify!
			// This seems to indicate that the remote peer closed the
			// connection.
			PRINT(("Connection::Send(): sent 0 bytes. Assuming "
				"that the remote peer closed the connection\n"));
			return SendError(B_ERROR);
		} else {
			size -= bytesSent;
			buffer += bytesSent;
//			remainingRetries = kMaxInterruptedTxCount;
PRINT(("SSLChannel::Send(): sent %d bytes, still to sent: %lu\n",
bytesSent, size));
		}
	}

	// we need to explicitely flush the data
	int cryptError = cryptFlushData(fSession);
	if (cryptError != CRYPT_OK) {
		if (cryptError == CRYPT_ERROR_TIMEOUT)
			return SendError(B_TIMED_OUT);
		return SendError(B_ERROR);
	}

	return B_OK;
}

// Receive
status_t
SSLChannel::Receive(void* _buffer, int32 size)
{
	if (size == 0)
		return B_OK;
	if (!_buffer || size < 0)
		return B_BAD_VALUE;

	uint8* buffer = static_cast<uint8*>(_buffer);
	int32 remainingRetries = kMaxInterruptedTxCount;
	while (size > 0) {
		int bytesRead;
		int cryptError = cryptPopData(fSession, buffer, size, &bytesRead);
		if (cryptError != CRYPT_OK) {
			if (cryptError == CRYPT_ERROR_TIMEOUT)
				return ReceiveError(B_TIMED_OUT);
			return ReceiveError(B_ERROR);
		}

		if (bytesRead == 0) {
// TODO: Verify!
			// This seems to indicate that the remote peer closed the
			// connection.
			PRINT(("Connection::Receive(): received 0 bytes. Assuming "
				"that the remote peer closed the connection\n"));
			return ReceiveError(B_ERROR);
		} else {
			size -= bytesRead;
			buffer += bytesRead;
			remainingRetries = kMaxInterruptedTxCount;
//PRINT(("SSLChannel::Receive(): received %ld bytes, still to read: %lu\n",
//bytesRead, size));
		}
	}
	return B_OK;
}

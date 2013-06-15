/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <new>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <cryptlib.h>

#include "Compatibility.h"
#include "Debug.h"
#include "HashString.h"
#include "NetAddress.h"
#include "NetFSDefs.h"
#include "SSLChannel.h"
#include "SSLConnection.h"


// constructor
SSLConnection::SSLConnection()
{
}

// destructor
SSLConnection::~SSLConnection()
{
}

// OpenClientChannel
status_t
SSLConnection::OpenClientChannel(in_addr serverAddr, uint16 port,
	Channel** _channel)
{
PRINT(("SSLConnection::OpenClientChannel(port: %hu)\n", port));
	// get a server address string
	NetAddress address;
	address.SetIP(ntohl(serverAddr.s_addr));
	address.SetPort(port);
	HashString addressString;
	status_t error = address.GetString(&addressString, true);
	if (error != B_OK)
		RETURN_ERROR(error);
PRINT(("SSLConnection::OpenClientChannel(): %s\n", addressString.GetString()));
	
	// create a session
	CRYPT_SESSION session;
	int cryptError = cryptCreateSession(&session, CRYPT_UNUSED,
		CRYPT_SESSION_SSL);
	if (cryptError != CRYPT_OK)
		RETURN_ERROR(B_ERROR);

	// set session server address
	cryptError = cryptSetAttributeString(session, CRYPT_SESSINFO_SERVER_NAME,
		addressString.GetString(), addressString.GetLength());
	if (cryptError != CRYPT_OK) {
		cryptDestroySession(session);
		RETURN_ERROR(B_ERROR);
	}

cryptError = cryptSetAttribute(session, CRYPT_OPTION_NET_READTIMEOUT, INT_MAX);
cryptError = cryptSetAttribute(session, CRYPT_OPTION_NET_WRITETIMEOUT, INT_MAX);

	// activate the session
	cryptError = cryptSetAttribute(session, CRYPT_SESSINFO_ACTIVE, 1);
	if (cryptError != CRYPT_OK) {
		cryptDestroySession(session);
		if (cryptError == CRYPT_ERROR_TIMEOUT)
			return B_TIMED_OUT;
PRINT(("activating the session failed: %d\n", cryptError));
int errorLocus;
int errorType;
cryptGetAttribute(session, CRYPT_ATTRIBUTE_ERRORLOCUS, &errorLocus);
cryptGetAttribute(session, CRYPT_ATTRIBUTE_ERRORTYPE, &errorType);
PRINT(("error locus: %d, type: %d\n", errorLocus, errorType));
int errorCode;
if (cryptGetAttribute(session, CRYPT_ATTRIBUTE_INT_ERRORCODE, &errorCode) == CRYPT_OK)
PRINT(("error code: %d\n", errorCode));
int errorStringLen;
char* errorString;
if (cryptGetAttributeString(session, CRYPT_ATTRIBUTE_INT_ERRORMESSAGE, &errorString, &errorStringLen) == CRYPT_OK)
PRINT(("error string: %.*s\n", errorStringLen, errorString));

		RETURN_ERROR(B_ERROR);
	}

	// create the channel
	Channel* channel = new(std::nothrow) SSLChannel(session);
	if (!channel) {
		cryptDestroySession(session);
		RETURN_ERROR(B_NO_MEMORY);
	}
	*_channel = channel;
	return B_OK;
}

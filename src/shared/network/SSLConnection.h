/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_SSL_CONNECTION_H
#define NET_FS_SSL_CONNECTION_H

#include "SocketConnection.h"

// SSLConnection
class SSLConnection : public SocketConnection {
public:
								SSLConnection();
	virtual						~SSLConnection();

protected:
	virtual	status_t			OpenClientChannel(in_addr serverAddr,
									uint16 port, Channel** channel);
};

#endif	// NET_FS_SSL_CONNECTION_H

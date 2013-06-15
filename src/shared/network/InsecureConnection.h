/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_INSECURE_CONNECTION_H
#define NET_FS_INSECURE_CONNECTION_H

#include "SocketConnection.h"

// InsecureConnection
class InsecureConnection : public SocketConnection {
public:
								InsecureConnection();
	virtual						~InsecureConnection();

protected:
	virtual	status_t			OpenClientChannel(in_addr serverAddr,
									uint16 port, Channel** channel);
};

#endif	// NET_FS_INSECURE_CONNECTION_H

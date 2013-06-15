/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_CONNECTION_H
#define NET_FS_CONNECTION_H

#include <OS.h>

class Channel;

// Connection
class Connection {
protected:
								Connection();

public:
	virtual						~Connection();

	virtual	status_t			Init(const char* clientID,
									const char* parameters) = 0;
	virtual	void				Close() = 0;

	virtual	Channel*			DownStreamChannel() const = 0;
	virtual	Channel*			UpStreamChannel() const = 0;

	virtual	int32				ProtocolVersion() const = 0;
};

#endif	// NET_FS_CONNECTION_H

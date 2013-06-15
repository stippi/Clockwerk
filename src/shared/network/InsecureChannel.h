/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_INSECURE_CHANNEL_H
#define NET_FS_INSECURE_CHANNEL_H

#include "Channel.h"

class NetAddress;

class InsecureChannel : public Channel {
public:
								InsecureChannel(int socket);
	virtual						~InsecureChannel();

	virtual	void				Close();
	virtual	bool				IsClosed() const;

	virtual	status_t			Send(const void* buffer, int32 size);
	virtual	status_t			Receive(void* buffer, int32 size);

			status_t			GetPeerAddress(NetAddress *address) const;

private:
			vint32				fSocket;
			bool				fClosed;
};

#endif	// NET_FS_INSECURE_CHANNEL_H

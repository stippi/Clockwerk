/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_SSL_CHANNEL_H
#define NET_FS_SSL_CHANNEL_H

#include <cryptlib.h>

#include "Channel.h"

class NetAddress;

class SSLChannel : public Channel {
public:
								SSLChannel(CRYPT_SESSION session);
	virtual						~SSLChannel();

	virtual	void				Close();
	virtual	bool				IsClosed() const;

	virtual	status_t			Send(const void* buffer, int32 size);
	virtual	status_t			Receive(void* buffer, int32 size);

private:
			CRYPT_SESSION		fSession;
			vint32				fClosed;
};

#endif	// NET_FS_SSL_CHANNEL_H

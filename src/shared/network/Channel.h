/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_CHANNEL_H
#define NET_FS_CHANNEL_H

#include <SupportDefs.h>


class Channel {
protected:
								Channel();

public:
	virtual						~Channel();

	virtual	void				Close() = 0;
	virtual	bool				IsClosed() const = 0;

	virtual	status_t			Send(const void* buffer, int32 size) = 0;
	virtual	status_t			Receive(void* buffer, int32 size) = 0;

protected:
			status_t			SendError(status_t error);
			status_t			ReceiveError(status_t error);
};

#endif	// NET_FS_CHANNEL_H

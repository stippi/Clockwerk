/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_ABSTRACT_CONNECTION_H
#define NET_FS_ABSTRACT_CONNECTION_H

#include <Locker.h>

#include "Connection.h"
#include "Vector.h"

class AbstractConnection : public Connection {
protected:
								AbstractConnection();

public:
	virtual						~AbstractConnection();

	virtual	status_t			Init(const char* clientID,
									const char* parameters) = 0;
			status_t			Init();
	virtual	void				Close();

			void				SetDownStreamChannel(Channel* channel);
			void				SetUpStreamChannel(Channel* channel);

	virtual	Channel*			DownStreamChannel() const;
	virtual	Channel*			UpStreamChannel() const;

	virtual	int32				ProtocolVersion() const;

protected:
			status_t			fInitStatus;
			Channel*			fDownStreamChannel;
			Channel*			fUpStreamChannel;
			int32				fProtocolVersion;
};

#endif	// NET_FS_ABSTRACT_CONNECTION_H

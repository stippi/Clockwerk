/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NET_FS_CONNECTION_FACTORY_H
#define NET_FS_CONNECTION_FACTORY_H

#include <SupportDefs.h>

class Connection;
class ConnectionListener;

class ConnectionFactory {
public:
								ConnectionFactory();
								~ConnectionFactory();

			status_t			CreateConnection(const char* type,
									const char* clientID,
									const char* parameters,
									Connection** connection);
};

#endif	// NET_FS_CONNECTION_FACTORY_H

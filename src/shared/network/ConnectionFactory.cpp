/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <string.h>

#include "ConnectionFactory.h"
#include "InsecureConnection.h"
#include "SSLConnection.h"

// constructor
ConnectionFactory::ConnectionFactory()
{
}

// destructor
ConnectionFactory::~ConnectionFactory()
{
}

// CreateConnection
status_t
ConnectionFactory::CreateConnection(const char* type, const char* clientID,
	const char* parameters, Connection** _connection)
{
	if (!type || !clientID)
		return B_BAD_VALUE;

	// create the connection
	Connection* connection = NULL;
	if (strcmp(type, "insecure") == 0)
		connection = new(std::nothrow) InsecureConnection;
	else if (strcmp(type, "ssl") == 0)
		connection = new(std::nothrow) SSLConnection;
	else
		return B_BAD_VALUE;
	if (!connection)
		return B_NO_MEMORY;

	// init it
	status_t error = connection->Init(clientID, parameters);
	if (error != B_OK) {
		delete connection;
		return error;
	}
	*_connection = connection;
	return B_OK;
}


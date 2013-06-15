/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <new>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "Compatibility.h"
#include "Debug.h"
#include "InsecureChannel.h"
#include "InsecureConnection.h"
#include "NetAddress.h"
#include "NetFSDefs.h"


// constructor
InsecureConnection::InsecureConnection()
{
}

// destructor
InsecureConnection::~InsecureConnection()
{
}

// OpenClientChannel
status_t
InsecureConnection::OpenClientChannel(in_addr serverAddr, uint16 port,
	Channel** _channel)
{
	// create a socket
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return errno;

	// connect
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr = serverAddr;
	if (connect(fd, (sockaddr*)&addr, sizeof(addr)) < 0) {
		status_t error = errno;
		closesocket(fd);
		RETURN_ERROR(error);
	}

	// create the channel
	Channel* channel = new(std::nothrow) InsecureChannel(fd);
	if (!channel) {
		closesocket(fd);
		return B_NO_MEMORY;
	}
	*_channel = channel;
	return B_OK;
}

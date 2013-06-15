/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef UTILS_H
#define UTILS_H

#include "Compatibility.h"

#ifdef BEOS_NETSERVER
#	include <socket.h>
#else
#	include <unistd.h>
#	include <sys/socket.h>
#endif

#include <SupportDefs.h>


template<typename T> T max(const T& a, const T& b) { return (a > b ? a : b); }
template<typename T> T min(const T& a, const T& b) { return (a < b ? a : b); }

// safe_closesocket
/*!	There seems to be race condition on a net_server system, if two threads
	try to close the same socket at the same time. This is work-around. The
	variable which stores the socket ID must be a vint32.
*/
static inline
void
safe_closesocket(vint32& socketVar)
{
	int32 socket = atomic_or(&socketVar, -1);
	if (socket >= 0) {
#ifndef BEOS_NETSERVER
#	ifndef __HAIKU__
// TODO: This doesn't seem to exist on Haiku and I don't know if it's
// necessary. Confirm we can do without.
		shutdown(socket, SHUTDOWN_BOTH);
#	endif
#endif
		closesocket(socket);
	}
}

#endif	// UTILS_H

/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <new>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

#include <ByteOrder.h>
#include <Locker.h>

#include "AutoLocker.h"
#include "Compatibility.h"
#include "HashString.h"
#include "NetAddress.h"
#include "Referenceable.h"

#if !BEOS_NETSERVER
#	include <arpa/inet.h>
#endif


// constructor
NetAddress::NetAddress()
{
	fAddress.sin_family = AF_INET;
	fAddress.sin_addr.s_addr = 0;
	fAddress.sin_port = 0;
}

// constructor
NetAddress::NetAddress(const sockaddr_in& address)
{
	fAddress = address;
}

// copy constructor
NetAddress::NetAddress(const NetAddress& address)
{
	fAddress = address.fAddress;
}

// SetIP
void
NetAddress::SetIP(int32 address)
{
	fAddress.sin_addr.s_addr = B_HOST_TO_BENDIAN_INT32(address);
}

// GetIP
int32
NetAddress::GetIP() const
{
	return B_BENDIAN_TO_HOST_INT32(fAddress.sin_addr.s_addr);
}

// SetPort
void
NetAddress::SetPort(uint16 port)
{
	fAddress.sin_port = B_HOST_TO_BENDIAN_INT16(port);
}

// GetPort
uint16
NetAddress::GetPort() const
{
	return B_BENDIAN_TO_HOST_INT16(fAddress.sin_port);
}

// SetAddress
void
NetAddress::SetAddress(const sockaddr_in& address)
{
	fAddress = address;
}

// GetAddress
const sockaddr_in&
NetAddress::GetAddress() const
{
	return fAddress;
}

// IsLocal
bool
NetAddress::IsLocal() const
{
	// special address?
	if (fAddress.sin_addr.s_addr == INADDR_ANY
		|| fAddress.sin_addr.s_addr == INADDR_BROADCAST) {
		return false;
	}
	// create a socket and try to bind it to a port of this address
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0)
		return false;
	// bind it to a port
	sockaddr_in addr = fAddress;
	addr.sin_port = 0;
	bool result = (bind(fd, (sockaddr*)&addr, sizeof(addr)) == 0);
	closesocket(fd);
	return result;
}


// GetString
status_t
NetAddress::GetString(HashString* string, bool includePort) const
{
	if (!string)
		return B_BAD_VALUE;
	char buffer[32];
	uint32 ip = GetIP();
	if (includePort) {
		sprintf(buffer, "%lu.%lu.%lu.%lu:%hu",
			ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff,
			GetPort());
	} else {
		sprintf(buffer, "%lu.%lu.%lu.%lu",
			ip >> 24, (ip >> 16) & 0xff, (ip >> 8) & 0xff, ip & 0xff);
	}
	return (string->SetTo(buffer) ? B_OK : B_NO_MEMORY);
}

// GetHashCode
uint32
NetAddress::GetHashCode() const
{
	return (fAddress.sin_addr.s_addr * 31 + fAddress.sin_port);
}

// =
NetAddress&
NetAddress::operator=(const NetAddress& address)
{
	fAddress = address.fAddress;
	return *this;
}

// ==
bool
NetAddress::operator==(const NetAddress& address) const
{
	return (fAddress.sin_addr.s_addr == address.fAddress.sin_addr.s_addr
		&& fAddress.sin_port == address.fAddress.sin_port);
}

// !=
bool
NetAddress::operator!=(const NetAddress& address) const
{
	return !(*this == address);
}


// #pragma mark -

// Resolver
class NetAddressResolver::Resolver : public Referenceable {
public:
	Resolver()
		: Referenceable(false),
		  fLock()
	{
	}

	status_t InitCheck() const
	{
		return (fLock.Sem() >= 0 ? B_OK : B_NO_INIT);
	}

	status_t GetHostAddress(const char* hostName, NetAddress* address)
	{
		AutoLocker<BLocker> _(fLock);
		struct hostent* host = gethostbyname(hostName);
		if (!host)
			return h_errno;
		if (host->h_addrtype != AF_INET || !host->h_addr_list[0])
			return B_BAD_VALUE;
		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_port = 0;
		addr.sin_addr = *(in_addr*)host->h_addr_list[0];
		*address = addr;
		return B_OK;
	}

private:
	BLocker	fLock;
};

// constructor
NetAddressResolver::NetAddressResolver()
{
	_Lock();
	// initialize static instance, if not done yet
	if (sResolver) {
		sResolver->AddReference();
		fResolver = sResolver;
	} else {
		sResolver = new(std::nothrow) Resolver;
		if (sResolver) {
			if (sResolver->InitCheck() != B_OK) {
				delete sResolver;
				sResolver = NULL;
			}
		}
		fResolver = sResolver;
	}
	_Unlock();
}

// destructor
NetAddressResolver::~NetAddressResolver()
{
	if (fResolver) {
		_Lock();
		if (sResolver->RemoveReference()) {
			delete sResolver;
			sResolver = NULL;
		}
		_Unlock();
	}
}

// InitCheck
status_t
NetAddressResolver::InitCheck() const
{
	return (fResolver ? B_OK : B_NO_INIT);
}

// GetAddress
status_t
NetAddressResolver::GetHostAddress(const char* hostName, NetAddress* address)
{
	if (!fResolver)
		return B_NO_INIT;
	if (!hostName || !address)
		return B_BAD_VALUE;

#if !BEOS_NETSERVER
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = 0;
	if (inet_aton(hostName, &addr.sin_addr) != 0) {
		*address = addr;
		return B_OK;
	}
#endif

	return fResolver->GetHostAddress(hostName, address);
}

// _Lock
void
NetAddressResolver::_Lock()
{
	while (atomic_add(&sLockCounter, 1) > 0) {
		atomic_add(&sLockCounter, -1);
		snooze(10000);
	}
}

// _Unlock
void
NetAddressResolver::_Unlock()
{
	atomic_add(&sLockCounter, -1);
}


// sResolver
NetAddressResolver::Resolver* volatile NetAddressResolver::sResolver = NULL;

// sLockCounter
vint32 NetAddressResolver::sLockCounter = 0;
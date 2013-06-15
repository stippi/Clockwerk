/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ServerList.h"

#include <ctype.h>
#include <new>
#include <unistd.h>
#include <stdio.h>

#include <Entry.h>
#include <File.h>

#include "common.h"


using std::nothrow;

static const int32 kMaxServers = 3;

// constructor
ServerList::ServerList(const char* pathToSettingsFile)
	: fServers(20)
	, fSettingsPath(pathToSettingsFile)
{
	_Load();
}

// destructor
ServerList::~ServerList()
{
	int32 count = CountServers();
	for (int32 i = 0; i < count; i++)
		delete (BString*)fServers.ItemAtFast(i);
}

// AddServer
status_t
ServerList::AddServer(const char* _serverAddress)
{
	// sanity checks
	if (!_serverAddress) {
		print_error("ServerList::AddServer(NULL)!\n");
		return B_BAD_VALUE;
	}

	if (strlen(_serverAddress) < 8) {
		print_error("ServerList::AddServer() - address too short\n");
		return B_BAD_VALUE;
	}

	// TODO: more IP formats? allow named servers?
	int a, b, c, d;
	if (sscanf(_serverAddress, "%d.%d.%d.%d", &a, &b, &c, &d) != 4
		|| a < 0 || a > 254 || b < 0 || b > 254
		|| c < 0 || c > 254 || d < 0 || d > 254) {
		print_error("ServerList::AddServer() - address not valid IP\n");
		return B_BAD_VALUE;
	}

	// clean up the address
	char serverAddress[strlen(_serverAddress) + 5];
	sprintf(serverAddress, "%d.%d.%d.%d", a, b, c, d);

	// check if the last server entry happens to be the
	// address we want to add
	int32 count = CountServers();
	if (count > 0) {
		BString* server = (BString*)fServers.ItemAtFast(count - 1);
		if (*server == serverAddress)
			return B_OK;
	}

	// check success of all related allocations
	BString* server = new (nothrow) BString(serverAddress);
	if (!server || *server != serverAddress || !fServers.AddItem(server)) {
		delete server;
		print_error("ServerList::AddServer() - out of memory\n");
		return B_NO_MEMORY;
	}

	// check all previous servers and remove a possible duplicate
	// NOTE: since we didn't increment "count", we are not removing
	// the server we just added!
	for (int32 i = 0; i < count; i++) {
		BString* _server = (BString*)fServers.ItemAtFast(i);
		if (*_server == serverAddress) {
			fServers.RemoveItem(i);
			delete _server;
			break;
		}
	}

	// keep the list short so that it is better controlled which
	// servers a client will try to connect with
	int32 tooMany = fServers.CountItems() - kMaxServers;
	while (tooMany > 0) {
		delete (BString*)fServers.RemoveItem(0L);
		tooMany--;
	}

	return B_OK;
}

// CountServers
int32
ServerList::CountServers() const
{
	return fServers.CountItems();
}

// ServerAt
const char*
ServerList::ServerAt(int32 index) const
{
	if (BString* server = (BString*)fServers.ItemAt(index))
		return server->String();
	return NULL;
}

// Save
status_t
ServerList::Save()
{
	if (fSettingsPath.Length() == 0) {
		print_error("ServerList::Save() - no path given!\n");
		return B_NO_INIT;
	}

	// use BEntry in order to traverse symbolic links
	BEntry entry(fSettingsPath.String(), true);

	status_t ret = entry.InitCheck();
	if (ret < B_OK) {
		print_error("ServerList::Save() - entry init failed:\n",
			strerror(ret));
		return ret;
	}

	BFile file(&entry, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	ret = file.InitCheck();
	if (ret < B_OK) {
		print_error("ServerList::Save() - file init failed:\n",
			strerror(ret));
		return ret;
	}

	ret = B_OK;
	int32 count = CountServers();
	for (int32 i = 0; i < count; i++) {
		BString* server = (BString*)fServers.ItemAtFast(i);
		// append '\n'
		ssize_t size = server->Length() + 1;
		char buffer[size];
		memcpy(buffer, server->String(), size - 1);
		buffer[size - 1] = '\n';
		ssize_t written = file.Write(buffer, size);
		if (written != size) {
			if (written < 0)
				ret = (status_t)written;
			else
				ret = B_ERROR;
			print_error("ServerList::Save() - "
				"failed to write file: %s\n", strerror(ret));
			break;
		}
	}
	sync();
	return ret;
}

// #pragma mark -

// _Load
status_t
ServerList::_Load()
{
	if (fSettingsPath.Length() == 0) {
		print_error("ServerList::_Load() - no path given!\n");
		return B_NO_INIT;
	}

	// use BEntry in order to traverse symbolic links
	BEntry entry(fSettingsPath.String(), true);

	status_t ret = entry.InitCheck();
	if (ret < B_OK) {
		print_error("ServerList::_Load() - entry init failed:\n",
			strerror(ret));
		return ret;
	}

	BFile file(&entry, B_READ_ONLY);
	ret = file.InitCheck();
	if (ret < B_OK) {
		print_error("ServerList::_Load() - file init failed:\n",
			strerror(ret));
		return ret;
	}

	size_t size = 1024;
	char buffer[size];
	char* bufferPos = buffer;
	ssize_t read = file.Read(buffer, size);
	BString line;
	while (read > 0) {
		// remove white spaces from beginning of buffer
		while (isspace(*bufferPos) && *bufferPos != '\n' && read > 0) {
			bufferPos++;
			read--;
		}
		// check if end of buffer already
		if (read == 0) {
			read = file.Read(buffer, size);
			bufferPos = buffer;
			if (read <= 0)
				break;
			continue;
		}

		// read until end of line
		char* charPos = bufferPos;
		ssize_t bytesParsed = 0;
		bool endOfLine = false;
		while (bytesParsed < read) {
			bytesParsed++;
			if (*charPos == '\n') {
				charPos--;
				endOfLine = true;
				break;
			}
			if (bytesParsed < read)
				charPos++;
		}
		ssize_t appendLength = charPos - bufferPos + 1;
		if (appendLength > 0) {
			char temp[appendLength + 1];
			memcpy(temp, bufferPos, appendLength);
			temp[appendLength] = 0;
			line << temp;
		}

		if (endOfLine && line.Length() > 0) {
			ret = AddServer(line.String());
			if (ret < B_OK)
				return ret;
			line = "";
		}

		bufferPos += bytesParsed;
		read -= bytesParsed;
		if (read == 0) {
			read = file.Read(buffer, size);
			bufferPos = buffer;
			if (read <= 0)
				break;
		}
	}
	if (line.Length() > 0) {
		// the file was not terminated by a '\n'
		// add what we have
		AddServer(line.String());
	}

	return B_OK;
}



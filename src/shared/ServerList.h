/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SERVER_LIST_H
#define SERVER_LIST_H

#include <List.h>
#include <String.h>

// AddServer() is supposed to be used additionally to the
// settings file. If AddServer() tries to add a server that
// is already listed, the index of this server will be moved
// to the end of the list.

class ServerList {
 public:
								ServerList(const char* pathToSettingsFile);
	virtual						~ServerList();

			status_t			AddServer(const char* serverAddress);

			int32				CountServers() const;
			const char*			ServerAt(int32 index) const;

			status_t			Save();
 private:
			status_t			_Load();

			BList				fServers;
			BString				fSettingsPath;
};

#endif // SERVER_LIST_H

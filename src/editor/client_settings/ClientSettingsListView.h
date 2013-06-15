/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIENT_SETTINGS_LIST_VIEW_H
#define CLIENT_SETTINGS_LIST_VIEW_H

#include "ObjectListView.h"

class ClientSettingsListView : public ObjectListView {
 public:
								ClientSettingsListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~ClientSettingsListView();

	// ObjectListView interface
	virtual	bool				AcceptObject(ServerObject* object);

};

#endif // CLIENT_SETTINGS_LIST_VIEW_H

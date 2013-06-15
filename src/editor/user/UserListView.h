/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef USER_LIST_VIEW_H
#define USER_LIST_VIEW_H

#include "ObjectListView.h"

class UserListView : public ObjectListView {
 public:
								UserListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~UserListView();

	// ObjectListView interface
	virtual	bool				AcceptObject(ServerObject* object);

};

#endif // USER_LIST_VIEW_H

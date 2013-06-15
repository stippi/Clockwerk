/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "UserListView.h"

#include <String.h>

#include "ServerObject.h"

// constructor
UserListView::UserListView(const char* name,
						   BMessage* message, BHandler* target)
	: ObjectListView(name, message, target)
{
}

// destructor
UserListView::~UserListView()
{
}

// AcceptObject
bool
UserListView::AcceptObject(ServerObject* object)
{
	return object->Type() == "User";
}

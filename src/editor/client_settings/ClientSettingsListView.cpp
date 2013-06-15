/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClientSettingsListView.h"

#include <String.h>

#include "ServerObject.h"

// constructor
ClientSettingsListView::ClientSettingsListView(const char* name,
											   BMessage* message,
											   BHandler* target)
	: ObjectListView(name, message, target)
{
}

// destructor
ClientSettingsListView::~ClientSettingsListView()
{
}

// AcceptObject
bool
ClientSettingsListView::AcceptObject(ServerObject* object)
{
	return object->Type() == "ClientSettings";
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DisplaySettingsListView.h"

#include <String.h>

#include "ServerObject.h"

// constructor
DisplaySettingsListView::DisplaySettingsListView(const char* name,
											   BMessage* message,
											   BHandler* target)
	: ObjectListView(name, message, target)
{
}

// destructor
DisplaySettingsListView::~DisplaySettingsListView()
{
}

// AcceptObject
bool
DisplaySettingsListView::AcceptObject(ServerObject* object)
{
	return object->Type() == "DisplaySettings";
}

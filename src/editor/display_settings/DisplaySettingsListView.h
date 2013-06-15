/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DISPLAY_SETTINGS_LIST_VIEW_H
#define DISPLAY_SETTINGS_LIST_VIEW_H

#include "ObjectListView.h"

class DisplaySettingsListView : public ObjectListView {
 public:
								DisplaySettingsListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~DisplaySettingsListView();

	// ObjectListView interface
	virtual	bool				AcceptObject(ServerObject* object);

};

#endif // DISPLAY_SETTINGS_LIST_VIEW_H

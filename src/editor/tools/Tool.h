/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TOOL_H
#define TOOL_H

#include <Handler.h>

class BMessage;
class ConfigView;
class IconButton;
class StateView;
class ViewState;

// NOTE: A Tool object is added to the MainWindow,
// which switches between tools. Each tool is also
// added to the BWindows handlers, so that BMessages
// can be sent to it.

class Tool : public BHandler {
 public:
								Tool(const char* name = NULL);
	virtual						~Tool();

	// save state
	virtual	status_t			SaveSettings(BMessage* message);
	virtual	status_t			LoadSettings(BMessage* message);

	// GUI
			::ConfigView*		ConfigView();
			IconButton*			Icon();

	virtual	const char*			ShortHelpMessage();

	// apply or cancel the changes of
	// more complex editing
	virtual	status_t			Confirm();
	virtual	status_t			Cancel();

 protected:
	virtual	::ConfigView*		MakeConfigView() = 0;
	virtual	IconButton*			MakeIcon() = 0;

			::ConfigView*		fConfigView;
			IconButton*			fIcon;
};

#endif	// TOOL_H

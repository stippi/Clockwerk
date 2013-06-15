/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PICK_TOOL_H
#define PICK_TOOL_H

#include "TimelineTool.h"

class PickTool : public TimelineTool {
 public:
								PickTool();
	virtual						~PickTool();

	// Tool interface
	virtual	status_t			SaveSettings(BMessage* message);
	virtual	status_t			LoadSettings(BMessage* message);

	virtual	const char*			ShortHelpMessage();

	// TimelineTool interface
	virtual	PlaylistItemManipulator*
								ManipulatorFor(PlaylistItem* item);

 protected:
	virtual	::ConfigView*		MakeConfigView();
	virtual	IconButton*			MakeIcon();
};

#endif	// PICK_TOOL_H

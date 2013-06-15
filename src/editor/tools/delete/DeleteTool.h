/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DELETE_TOOL_H
#define DELETE_TOOL_H

#include "TimelineTool.h"

class DeleteTool : public TimelineTool {
 public:
								DeleteTool();
	virtual						~DeleteTool();

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

#endif	// DELETE_TOOL_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIME_LINE_TOOL_H
#define TIME_LINE_TOOL_H

#include "Tool.h"

class PlaylistItem;
class PlaylistItemManipulator;

class TimelineTool : public Tool {
 public:
								TimelineTool(const char* name = NULL);
	virtual						~TimelineTool();

	virtual	PlaylistItemManipulator*
								ManipulatorFor(PlaylistItem* item) = 0;

};

#endif	// TIME_LINE_TOOL_H

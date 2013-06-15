/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef STAGE_TOOL_H
#define STAGE_TOOL_H

#include "Tool.h"

class MultipleManipulatorState;
class Playlist;
class PlaylistItem;
class StateView;
class VideoViewSelection;

class StageTool : public Tool {
 public:
								StageTool(const char* name = NULL);
	virtual						~StageTool();

	virtual	bool				AddManipulators(StateView* view,
									MultipleManipulatorState* viewState,
									PlaylistItem** const selectedItems,
									int32 count,
									Playlist* playlist,
									VideoViewSelection* stageSelection) = 0;

};

#endif	// STAGE_TOOL_H

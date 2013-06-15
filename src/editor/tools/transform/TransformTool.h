/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TRANSFORM_TOOL_H
#define TRANSFORM_TOOL_H

#include "StageTool.h"

class TransformTool : public StageTool {
 public:
								TransformTool();
	virtual						~TransformTool();

	// Tool interface
	virtual	status_t			SaveSettings(BMessage* message);
	virtual	status_t			LoadSettings(BMessage* message);

	virtual	const char*			ShortHelpMessage();

	// StageTool interface
	virtual	bool				AddManipulators(StateView* view,
									MultipleManipulatorState* viewState,
									PlaylistItem** const items, int32 count,
									Playlist* playlist,
									VideoViewSelection* stageSelection);

 protected:
	virtual	::ConfigView*		MakeConfigView();
	virtual	IconButton*			MakeIcon();
};

#endif	// TRANSFORM_TOOL_H

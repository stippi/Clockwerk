/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EDIT_ON_STAGE_TOOL_H
#define EDIT_ON_STAGE_TOOL_H

#include "StageTool.h"

class EditOnStageTool : public StageTool {
 public:
								EditOnStageTool();
	virtual						~EditOnStageTool();

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

#endif // EDIT_ON_STAGE_TOOL_H

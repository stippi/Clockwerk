/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EDITOR_PLAYBACK_NAVIGATOR_H
#define EDITOR_PLAYBACK_NAVIGATOR_H

#include "PlaybackNavigator.h"

class EditorApp;

class EditorPlaybackNavigator : public PlaybackNavigator {
public:
								EditorPlaybackNavigator(EditorApp* app);
	virtual						~EditorPlaybackNavigator();

	virtual	void				Navigate(const NavigationInfo* info);

private:
			EditorApp*			fApplication;
};


#endif // EDITOR_PLAYBACK_NAVIGATOR_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYBACK_NAVIGATOR_H
#define PLAYBACK_NAVIGATOR_H

#include <SupportDefs.h>

class NavigationInfo;

class PlaybackNavigator {
public:
								PlaybackNavigator();
	virtual						~PlaybackNavigator();

	static	void				SetDefault(PlaybackNavigator* navigator);
	static	void				DeleteDefault();
	static	PlaybackNavigator*	Default();

	virtual	void				Navigate(const NavigationInfo* info);

private:
	static	PlaybackNavigator*	sDefaultInstance;
};


#endif // PLAYBACK_NAVIGATOR_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYER_VIDEO_VIEW_H
#define PLAYER_VIDEO_VIEW_H

#include <View.h>

class PlayerPlaybackNavigator;
class Playlist;

class PlayerVideoView : public BView {
public:
								PlayerVideoView(BRect frame, bool testMode);
	virtual						~PlayerVideoView();

	virtual	void				AttachedToWindow();
	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

			void				SetVideoSize(uint32 width, uint32 height);
			void				SetNavigator(
									PlayerPlaybackNavigator* navigator);
			void				SetPlaylist(Playlist* playlist);

private:
			PlayerPlaybackNavigator* fNavigator;
			Playlist*			fPlaylist;
			uint32				fVideoWidth;
			uint32				fVideoHeight;
			bool				fTestMode;
};

#endif // PLAYER_VIDEO_VIEW_H

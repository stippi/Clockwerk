/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SLIDE_SHOW_PLAYLIST_H
#define SLIDE_SHOW_PLAYLIST_H

#include "Playlist.h"

class SlideShowPlaylist : public Playlist {
 public:
								SlideShowPlaylist();
								SlideShowPlaylist(
									const SlideShowPlaylist& other);
	virtual						~SlideShowPlaylist();

	// Playlist interface
	virtual	void				ValidateItemLayout();
};

#endif // SLIDE_SHOW_PLAYLIST_H

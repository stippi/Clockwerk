/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef STRETCHING_PLAYLIST_H
#define STRETCHING_PLAYLIST_H

#include "Playlist.h"

class StretchingPlaylist : public Playlist {
 public:
								StretchingPlaylist(const char* type = NULL);
								StretchingPlaylist(
									const StretchingPlaylist& other);
	virtual						~StretchingPlaylist();

	// Playlist interface
	virtual	void				ValidateItemLayout();
};

#endif // STRETCHING_PLAYLIST_H

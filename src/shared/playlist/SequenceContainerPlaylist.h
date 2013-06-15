/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SEQUENCE_CONTAINER_PLAYLIST_H
#define SEQUENCE_CONTAINER_PLAYLIST_H

#include "Playlist.h"

class SequenceContainerPlaylist : public Playlist {
 public:
								SequenceContainerPlaylist();
								SequenceContainerPlaylist(
									const SequenceContainerPlaylist& other);
	virtual						~SequenceContainerPlaylist();

	// Playlist interface
	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

	virtual	void				ValidateItemLayout();

 protected:
	virtual	void				_ItemsChanged();

 private:
 			bool				fItemLayoutValid;
};

#endif // SEQUENCE_CONTAINER_PLAYLIST_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COLLECTING_PLAYLIST_H
#define COLLECTING_PLAYLIST_H

#include "Playlist.h"

class CollectingPlaylist : public Playlist {
 public:
								CollectingPlaylist();
								CollectingPlaylist(
									const CollectingPlaylist& other);
	virtual						~CollectingPlaylist();

	// Clip interface
//	virtual	uint64				MaxDuration();

	// Playlist interface
	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

	// CollectingPlaylist
			void				SetTransitionClipID(
									const char* transitionClipID);
			const char*			TransitionClipID() const;

			void				SetSoundClipID(const char* soundClipID);
			const char*			SoundClipID() const;

			void				SetTypeMarker(const char* typeMarker);
			const char*			TypeMarker() const;

			void				SetItemDuration(uint64 duration);
			uint64				ItemDuration() const;

			int32				CollectorMode() const;
 private:
			status_t			_CollectSequence(BList& collectables,
									const ServerObjectManager* library);
			status_t			_CollectRandom(BList& collectables,
									const ServerObjectManager* library);
			status_t			_LayoutBackgroundSound(
									const ServerObjectManager* library);
};

#endif // COLLECTING_PLAYLIST_H

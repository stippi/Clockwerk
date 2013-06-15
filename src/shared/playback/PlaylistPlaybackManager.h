/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PLAYLIST_PLAYBACK_MANAGER_H
#define PLAYLIST_PLAYBACK_MANAGER_H

#include "NodeManager.h"
#include "PlaylistLOAdapter.h"

class Playlist;
class PlaylistAudioSupplier;
class PlaylistVideoSupplier;
class RWLocker;

class PlaylistPlaybackManager : public NodeManager {
 public:
								PlaylistPlaybackManager(VCTarget* target,
														RWLocker* locker);
	virtual						~PlaylistPlaybackManager();

	// BLooper interface
	virtual	void				MessageReceived(BMessage* message);

	// PlaybackManager interface
	virtual	int64				Duration();

	// NodeManager interface
	virtual	VCTarget*			CreateVCTarget();
	virtual	VideoSupplier*		CreateVideoSupplier();
	virtual	AudioSupplier*		CreateAudioSupplier();

	virtual	void				SetVolume(float percent);

	// PlaylistPlaybackManager
			void				SetPlaylist(Playlist* playlist);

 private:
			Playlist*			fPlaylist;
			PlaylistVideoSupplier* fVideoSupplier;
			PlaylistAudioSupplier* fAudioSupplier;
			VCTarget*			fVCTarget;
				// VCTarget stands for "VideoConsumerTarget"
				// it is the view that is used for displaying
				// the video bitmap
			RWLocker*			fLocker;
				// (optional) additional readlocker
				// to be passed on to the suppliers
				// so that they readlock before accessing
				// data
			PlaylistLOAdapter	fPlaylistObserver;
};

#endif // PLAYLIST_PLAYBACK_MANAGER_H

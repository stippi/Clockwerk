/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistPlaybackManager.h"

#include <stdio.h>

#include <Message.h>

#include "Playlist.h"
#include "PlaylistAudioSupplier.h"
#include "PlaylistVideoSupplier.h"

// constructor
PlaylistPlaybackManager::PlaylistPlaybackManager(VCTarget* target,
												 RWLocker* locker)
	: NodeManager(),
	  fPlaylist(NULL),
	  fVideoSupplier(new PlaylistVideoSupplier(NULL, locker)),
	  fAudioSupplier(NULL),
	  fVCTarget(target),

	  fLocker(locker),

	  fPlaylistObserver(this)
{
}

// destructor
PlaylistPlaybackManager::~PlaylistPlaybackManager()
{
	SetPlaylist(NULL);
}

// MessageReceived
void
PlaylistPlaybackManager::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAYLIST_ITEM_ADDED:
//			printf("MSG_PLAYLIST_ITEM_ADDED\n");
			break;
		case MSG_PLAYLIST_ITEM_REMOVED:
//			printf("MSG_PLAYLIST_ITEM_REMOVED\n");
			break;
		case MSG_PLAYLIST_DURATION_CHANGED:
//			printf("MSG_PLAYLIST_DURATION_CHANGED\n");
			DurationChanged();
			break;

		default:
			NodeManager::MessageReceived(message);
			break;
	}
}

// Duration
int64
PlaylistPlaybackManager::Duration()
{
	if (fPlaylist)
		return fPlaylist->Duration();
	return 0;
}

// CreateVCTarget
VCTarget*
PlaylistPlaybackManager::CreateVCTarget()
{
	return fVCTarget;
}

// CreateVideoSupplier
VideoSupplier*
PlaylistPlaybackManager::CreateVideoSupplier()
{
	return fVideoSupplier;
}

// CreateAudioSupplier
AudioSupplier*
PlaylistPlaybackManager::CreateAudioSupplier()
{
	fAudioSupplier = new PlaylistAudioSupplier(fPlaylist, fLocker,
		this, FramesPerSecond());
	return fAudioSupplier;
}

// SetVolume
void
PlaylistPlaybackManager::SetVolume(float percent)
{
	if (fAudioSupplier)
		fAudioSupplier->SetVolume(percent);
}

// SetPlaylist
void
PlaylistPlaybackManager::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	if (fPlaylist)
		fPlaylist->RemoveListObserver(&fPlaylistObserver);

	fPlaylist = playlist;

	if (fVideoSupplier)
		fVideoSupplier->SetPlaylist(playlist);
	if (fAudioSupplier)
		fAudioSupplier->SetPlaylist(playlist);

	if (fPlaylist)
		fPlaylist->AddListObserver(&fPlaylistObserver);

	DurationChanged();
}

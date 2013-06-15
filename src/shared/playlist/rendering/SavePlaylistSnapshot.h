/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SAVE_PLAYLIST_SNAPSHOT_H
#define SAVE_PLAYLIST_SNAPSHOT_H

#include <String.h>

class Playlist;


struct PlaybackSnapShotInfo {
								PlaybackSnapShotInfo(Playlist* playlist,
									int64 currentFrame);
								~PlaybackSnapShotInfo();

			Playlist*			playlist;
			int64				currentFrame;
			BString				path;
				// defaults to "/boot/home/Desktop"
};

// launching a thread that will use the function below.
int32 save_playback_snapshot(void* playbackSnapShotInfo);

// direct saving of a Playlist snapshot as JPG to the given path
status_t save_playlist_snapshot(Playlist* playlist, int32 frame, BString& path);

#endif	// SAVE_PLAYLIST_SNAPSHOT_H

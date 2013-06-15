/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SavePlaylistSnapshot.h"

#include <stdio.h>
#include <string.h>

#include "AutoDeleter.h"
#include "Playlist.h"
#include "PlaylistRenderer.h"

#include "common.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <File.h>
#include <NodeInfo.h>
#include <TranslatorRoster.h>


PlaybackSnapShotInfo::PlaybackSnapShotInfo(Playlist* playlist,
		int64 currentFrame)
	: playlist(playlist)
	, currentFrame(currentFrame)
	, path("/boot/home/Desktop")
{
	if (playlist)
		playlist->Acquire();
}


PlaybackSnapShotInfo::~PlaybackSnapShotInfo()
{
	if (playlist)
		playlist->Release();
}


int32
save_playback_snapshot(void* cookie)
{
	PlaybackSnapShotInfo* info = (PlaybackSnapShotInfo*)cookie;
	ObjectDeleter<PlaybackSnapShotInfo> deleter(info);

	Playlist* playlist = info->playlist;
	if (!playlist) {
		print_error("save_playback_snapshot() - no playlist for snapshot\n");
		return B_ERROR;
	}	

	return save_playlist_snapshot(playlist, info->currentFrame, info->path);
}


status_t
save_playlist_snapshot(Playlist* playlist, int32 frame, BString& _path)
{
	if (!playlist) {
		printf("save_playlist_snapshot() - no playlist for snapshot\n");
		return B_BAD_VALUE;
	}	

	PlaylistRenderer renderer(playlist, playlist->Width(), playlist->Height());
	if (!renderer.IsValid()) {
		printf("save_playlist_snapshot() - failed to create "
			"PlaylistRenderer\n");
		return B_ERROR;
	}

	bool dummy;
	status_t ret = renderer.RenderFrame(frame, dummy);
	if (ret < B_OK) {
		printf("save_playlist_snapshot() - rendering failed: %s\n",
			strerror(ret));
		return ret;
	}

	char path[256];
	int32 seconds = frame / 25;
	int32 frames = frame - seconds * 25;
	int32 minutes = seconds / 60;
	int32 hours = seconds / (60 * 60);
	seconds -= hours * (60 * 60) + minutes * 60;
	sprintf(path, "%s/%s "
		"%ld_%0*ld_%0*ld_%0*ld.jpg", _path.String(), playlist->Name().String(),
		hours, 2, minutes, 2, seconds, 2, frames);

	BFile fileStream(path, B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
	ret = fileStream.InitCheck();
	if (ret < B_OK) {
		printf("save_playlist_snapshot() - failed to create output "
			"file: %s", strerror(ret));
		return ret;
	}

	BTranslatorRoster* tr = BTranslatorRoster::Default();

	BBitmap* bitmap = const_cast<BBitmap*>(renderer.Bitmap());
	BBitmapStream bitmapStream(bitmap);
	ret = tr->Translate(&bitmapStream, NULL, NULL, &fileStream, B_JPEG_FORMAT, 0);
	bitmapStream.DetachBitmap(&bitmap);
	if (ret < B_OK) {
		printf("save_playlist_snapshot() - failed to write output "
			"bitmap: %s", strerror(ret));
		return ret;
	}

	BNodeInfo nodeInfo(&fileStream);
	if (nodeInfo.InitCheck() >= B_OK)
		nodeInfo.SetType("image/jpeg");

	_path = path;

	return B_OK;
}


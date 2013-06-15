/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PLAYLIST_VIDEO_SUPPLIER_H
#define PLAYLIST_VIDEO_SUPPLIER_H

#include "VideoSupplier.h"
#include "ClipRendererCache.h"
#include "Painter.h"

class Playlist;
class RWLocker;

class PlaylistVideoSupplier : public VideoSupplier {
 public:
								PlaylistVideoSupplier(Playlist* list,
													  RWLocker* locker);
	virtual						~PlaylistVideoSupplier();

	virtual	status_t			FillBuffer(int64 startFrame,
										   void* buffer,
										   const media_format* format,
										   bool& wasCached);

			void				SetPlaylist(Playlist* playlist);

 private:
		 	Playlist*			fPlaylist;
			// for drawing into the video bitmaps
			Painter				fPainter;
			// (optional) additional readlocking
			// before accessing the playlist
			RWLocker*			fLocker;

			ClipRendererCache	fRendererCache;
};

#endif	// PLAYLIST_VIDEO_SUPPLIER_H

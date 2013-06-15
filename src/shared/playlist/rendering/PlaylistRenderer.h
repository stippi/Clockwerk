/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_RENDERER_H
#define PLAYLIST_RENDERER_H

#include <GraphicsDefs.h>
#include <List.h>
#include <MediaDefs.h>
#include <MediaFormats.h>

#include "ClipRendererCache.h"
#include "Painter.h"

class BBitmap;
class Playlist;

class PlaylistRenderer {
public:
								PlaylistRenderer(Playlist* playlist,
									uint32 width, uint32 height,
									uint32 flags = 0,
									color_space format = B_RGB32);
	virtual						~PlaylistRenderer();

			bool				IsValid() const;

	virtual	status_t			RenderFrame(int32 frame, bool& wasCached);

			const BBitmap*		Bitmap() const;

			status_t			GetNextVideoChunk(int32 frame,
									const void*& buffer, size_t& size,
									bool& chunksComplete,
									const media_codec_info& codecInfo);
			const media_header&	ChunkHeader() const;

private:
			class ChunkReaderSupport;

private:
 			Playlist*			fPlaylist;
 			Painter				fPainter;
 			ClipRendererCache	fRendererCache;
			BBitmap*			fCacheBitmap;
			uint32				fFlags;
			bool				fPrintError;
			ChunkReaderSupport*	fLastChunk;

};

#endif	// PLAYLIST_RENDERER_H

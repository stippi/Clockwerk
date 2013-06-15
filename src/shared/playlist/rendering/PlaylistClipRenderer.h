/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_CLIP_RENDERER_H
#define PLAYLIST_CLIP_RENDERER_H

#include "ClipRenderer.h"

class ClipRendererCache;
class Playlist;

class PlaylistClipRenderer : public ClipRenderer {
 public:
								PlaylistClipRenderer(ClipPlaylistItem* item,
									Playlist* playlist,
									ClipRendererCache* rendererCache);
	virtual						~PlaylistClipRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);

 private:
			Playlist*			fPlaylist;
			ClipRendererCache*	fRendererCache;
};

#endif // PLAYLIST_CLIP_RENDERER_H

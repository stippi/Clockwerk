/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef RENDER_PLAYLIST_H
#define RENDER_PLAYLIST_H

#include <GraphicsDefs.h>

#include "Playlist.h"

class BRegion;
class ClipRendererCache;
class Painter;

class RenderPlaylist : public Playlist {
 public:
								RenderPlaylist(const Playlist& other,
									double frame, color_space format,
									ClipRendererCache* rendererCache);
	virtual						~RenderPlaylist();

			status_t			Generate(Painter* painter, double frame);

			void				RemoveSolidRegion(BRegion* cleanBG,
									Painter* painter, double frame);
};

#endif // RENDER_PLAYLIST_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistClipRenderer.h"

#include <new>
#include <stdio.h>

#include "Painter.h"
#include "Playlist.h"
#include "RenderPlaylist.h"

using std::nothrow;

// constructor
PlaylistClipRenderer::PlaylistClipRenderer(ClipPlaylistItem* item,
		Playlist* playlist, ClipRendererCache* rendererCache)
	: ClipRenderer(item, playlist)
	, fPlaylist(new (nothrow) Playlist(*playlist, true))
	, fRendererCache(rendererCache)
{
}

// destructor
PlaylistClipRenderer::~PlaylistClipRenderer()
{
	if (fPlaylist)
		fPlaylist->Release();
}

// Generate
status_t
PlaylistClipRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	if (!fPlaylist)
		return B_NO_INIT;

	fPlaylist->SetCurrentFrame(frame);
	RenderPlaylist renderPlaylist(*fPlaylist, frame,
		(color_space)painter->PixelFormat(), fRendererCache);
	return renderPlaylist.Generate(painter, frame);
}


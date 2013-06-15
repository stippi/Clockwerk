/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipRenderer.h"

#include <stdio.h>

#include "ClipPlaylistItem.h"
#include "Playlist.h"

// constructor
ClipRenderer::ClipRenderer(ClipPlaylistItem* item, const ::Clip* clip)
	: fItem(item)
	, fClip(clip)
	, fReloadToken(clip->ChangeToken())
{
}

// destructor
ClipRenderer::~ClipRenderer()
{
}

// Generate
status_t
ClipRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	return B_ERROR;
}

// Sync
void
ClipRenderer::Sync()
{
	// should be implemented by derived classes to sync with changing
	// properties of the clip that they render, for example the text of
	// a ScrollingTextClip. The update is supposed to happen while the
	// Playlist/ClipLibrary is read-locked, IAW, don't access your
	// clip/item outside of Sync()

	fDuration = fItem ? fItem->Duration() : 0LL;
	fVideoFrameRate = fItem && fItem->Parent() ?
						fItem->Parent()->VideoFrameRate() : 25.0;
}

// IsSolid
bool
ClipRenderer::IsSolid(double frame) const
{
	return false;
}

// NeedsReload
bool
ClipRenderer::NeedsReload() const
{
	return fClip != fItem->Clip() || fClip->ChangeToken() != fReloadToken;
}


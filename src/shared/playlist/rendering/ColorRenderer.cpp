/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ColorRenderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "ui_defines.h"

#include "ColorClip.h"
#include "Painter.h"

// constructor
ColorRenderer::ColorRenderer(ClipPlaylistItem* item,
							 ColorClip* clip)
	: ClipRenderer(item, clip),
	  fClip(clip),
	  fColor(kBlack)
{
	if (fClip)
		fClip->Acquire();
}

// destructor
ColorRenderer::~ColorRenderer()
{
	if (fClip)
		fClip->Release();
}

// Generate
status_t
ColorRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	painter->SetColor(fColor);
	painter->FillRect(painter->Bounds());

	return B_OK;
}

// Sync
void
ColorRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip)
		fColor = fClip->Color();
}

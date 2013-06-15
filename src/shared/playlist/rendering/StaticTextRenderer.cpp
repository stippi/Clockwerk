/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "StaticTextRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Font.h>
#include <Message.h>
#include <MessageFilter.h>

#include "common_constants.h"
#include "ui_defines.h"

#include "Painter.h"
#include "TextBlockRenderer.h"
#include "TextClip.h"

using std::nothrow;

// constructor
StaticTextRenderer::StaticTextRenderer(ClipPlaylistItem* item, TextClip* clip)
	: ClipRenderer(item, clip),
	  fClip(clip),
	  fRenderer(new TextBlockRenderer()),
	  fColor(kWhite)
{
	if (fClip)
		fClip->Acquire();
}

// destructor
StaticTextRenderer::~StaticTextRenderer()
{
	if (fClip)
		fClip->Release();

	delete fRenderer;
}

// Generate
status_t
StaticTextRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	painter->SetColor(fColor);

	fRenderer->RenderText(*painter);

	return B_OK;
}

// Sync
void
StaticTextRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip) {
		Font font = fClip->Font();
		font.SetSize(fClip->FontSize());
		font.SetHinting(fClip->Hinting());
		fRenderer->SetFont(font);

		fRenderer->SetLayout(fClip->ParagraphInset(),
							 fClip->ParagraphSpacing(),
							 fClip->LineSpacing(),
							 fClip->GlyphSpacing(),
							 fClip->BlockWidth(),
							 fClip->Alignment());

		fRenderer->SetText(fClip->Text());

		fColor = fClip->Color();
	}
}


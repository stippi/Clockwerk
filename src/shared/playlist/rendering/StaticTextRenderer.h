/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef STATIC_TEXT_RENDERER_H
#define STATIC_TEXT_RENDERER_H

#include <Font.h>
#include <String.h>

#include "ClipRenderer.h"

class TextBlockRenderer;
class TextClip;

class StaticTextRenderer : public ClipRenderer {
 public:
								StaticTextRenderer(ClipPlaylistItem* item,
												   TextClip* clip);
	virtual						~StaticTextRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);

	virtual	void				Sync();

 private:
			TextClip*			fClip;

			TextBlockRenderer*	fRenderer;
			rgb_color			fColor;
};

#endif // STATIC_TEXT_RENDERER_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COLOR_RENDERER_H
#define COLOR_RENDERER_H

#include <GraphicsDefs.h>

#include "ClipRenderer.h"

class ColorClip;

class ColorRenderer : public ClipRenderer {
 public:
								ColorRenderer(ClipPlaylistItem* item,
											  ColorClip* clip);
	virtual						~ColorRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);

	virtual	void				Sync();

 private:
			ColorClip*			fClip;

			rgb_color			fColor;
};

#endif // COLOR_RENDERER_H

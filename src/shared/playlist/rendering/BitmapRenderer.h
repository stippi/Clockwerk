/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef BITMAP_RENDERER_H
#define BITMAP_RENDERER_H

#include <GraphicsDefs.h>

#include "ClipRenderer.h"

class BBitmap;
class BitmapClip;
class MemoryBuffer;

class BitmapRenderer : public ClipRenderer {
 public:
								BitmapRenderer(ClipPlaylistItem* item,
									BitmapClip* bitmapClip,
									color_space format);
	virtual						~BitmapRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	bool				IsSolid(double frame) const;

	virtual	void				Sync();

 private:
			void				_ConvertToYCbRr(const BBitmap* src,
												MemoryBuffer* dst);

			BitmapClip*			fClip;
			MemoryBuffer*		fBuffer;
			uint32				fFadeMode;
};

#endif // BITMAP_RENDERER_H

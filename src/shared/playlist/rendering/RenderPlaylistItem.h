/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef RENDER_PLAYLIST_ITEM_H
#define RENDER_PLAYLIST_ITEM_H

#include <GraphicsDefs.h>

#include "PlaylistItem.h"

class BRegion;
class ClipRenderer;
class ClipRendererCache;
class Painter;

class RenderPlaylistItem : public PlaylistItem {
public:
								RenderPlaylistItem(PlaylistItem* other,
									double frame, color_space format,
									ClipRendererCache* rendererCache);
	virtual						~RenderPlaylistItem();

	// PlaylistItem interface
	virtual	PlaylistItem*		Clone(bool deep) const;

	virtual	bool				HasVideo();
			bool				Generate(Painter* painter, double frame);
			ClipRenderer*		Renderer() const
									{ return fRenderer; }

	virtual	bool				HasAudio();
	virtual	AudioReader*		CreateAudioReader();

	virtual	BRect				Bounds(BRect canvasBounds,
									bool transformed = true);

			void				RemoveSolidRegion(BRegion* cleanBG,
									Painter* painter, double frame);

	virtual BString				Name() const;

	virtual	AffineTransform		Transformation() const;
	virtual	float				Alpha() const;

private:
			void				_CreateRenderer(color_space format,
									ClipRendererCache* rendererCache);
			float				_ValueAt(const Property* property,
									double frame,
									float defaultValue) const;

			PlaylistItem*		fOriginalItem;
			ClipRenderer*		fRenderer;

			float				fAlpha;
			AffineTransform		fTransformation;
};

#endif // RENDER_PLAYLIST_ITEM_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_RENDERER_H
#define CLIP_RENDERER_H

#include "AffineTransform.h"
#include "Referencable.h"

class Clip;
class ClipPlaylistItem;
class Painter;
class RenderPlaylistItem;

class ClipRenderer : public Referencable {
 public:
								ClipRenderer(ClipPlaylistItem* item,
									const ::Clip* clip);
	virtual						~ClipRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	void				Sync();
	virtual	bool				IsSolid(double frame) const;

	// ClipRenderer
			float				PlaylistVideoFrameRate() const
									{ return fVideoFrameRate; }
			int64				Duration() const
									{ return fDuration; }

			bool				NeedsReload() const;

	// debugging only:
			const ::Clip*		Clip() const
									{ return fClip; }

 private:
			ClipPlaylistItem*	fItem;

			const ::Clip*		fClip;
			uint32				fReloadToken;

			int64				fDuration;
			float				fVideoFrameRate;
};

#endif // CLIP_RENDERER_H

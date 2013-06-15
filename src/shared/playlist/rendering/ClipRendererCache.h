/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_RENDERER_CACHE_H
#define CLIP_RENDERER_CACHE_H


#include "HashMap.h"


class ClipRenderer;
class PlaylistItem;

static const int32 kDefaultUsageCount = 250; // 10 secs
	// this value determines the lifespan of a ClipRenderer,
	// you can't set it too high, or else there might be too
	// many files open at the same time (video)


class ClipRendererCache {
 public:
								ClipRendererCache();
	virtual						~ClipRendererCache();


			bool				AddRenderer(ClipRenderer* renderer,
									const PlaylistItem* item);
			void				RemoveRendererFor(const PlaylistItem* item);

			ClipRenderer*		RendererFor(const PlaylistItem* item) const;

			void				DeleteOldRenderers();

 private:
			struct CacheEntry {
								CacheEntry()
									: renderer(NULL)
									, useCounter(0)
								{
								}
								CacheEntry(const CacheEntry& other)
									: renderer(other.renderer)
									, useCounter(other.useCounter)
								{
								}
								CacheEntry(ClipRenderer* renderer)
									: renderer(renderer)
									, useCounter(kDefaultUsageCount)
								{
								}

				ClipRenderer*	renderer;
				int32			useCounter;
			};

			typedef HashMap<HashKey32<const PlaylistItem*>,
				CacheEntry*> RendererMap;

			RendererMap			fMap;
};


#endif // CLIP_RENDERER_CACHE_H

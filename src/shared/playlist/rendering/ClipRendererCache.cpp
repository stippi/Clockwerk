/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipRendererCache.h"

#include "Clip.h"
#include "ClipRenderer.h"

#include <new>
#include <stdio.h>


using std::nothrow;


// constructor
ClipRendererCache::ClipRendererCache()
	: fMap()
{
}

// destructor
ClipRendererCache::~ClipRendererCache()
{
	// we "own" the instances, meaning we dare to call Release()
	// without having called Acquire() anywhere
	RendererMap::Iterator iterator = fMap.GetIterator();
	while (iterator.HasNext()) {
		CacheEntry* entry = iterator.Next().value;
		entry->renderer->Release();
	}
}

// AddRenderer
bool
ClipRendererCache::AddRenderer(ClipRenderer* renderer,
	const PlaylistItem* item)
{
	if (fMap.ContainsKey(item)) {
		printf("ClipRendererCache::AddRenderer() - tried to add "
			"another renderer for the same PlaylistItem\n");
		return false;
	}

	CacheEntry* entry = new (nothrow) CacheEntry(renderer);
	if (!entry || fMap.Put(item, entry) < B_OK) {
		delete entry;
		return false;
	}

	return true;
}

// RemoveRendererFor
void
ClipRendererCache::RemoveRendererFor(const PlaylistItem* item)
{
	fMap.Remove(item);
}

// RendererFor
ClipRenderer*
ClipRendererCache::RendererFor(const PlaylistItem* item) const
{
	if (fMap.ContainsKey(item)) {
		CacheEntry* entry = fMap.Get(item);
		entry->useCounter = kDefaultUsageCount;
		return entry->renderer;
	}
	return NULL;
}

// DeleteOldRenderers
void
ClipRendererCache::DeleteOldRenderers()
{
	RendererMap::Iterator iterator = fMap.GetIterator();
	while (iterator.HasNext()) {
		CacheEntry* entry = iterator.Next().value;
		entry->useCounter--;
//printf("%s entry->useCounter: %ld\n", entry->renderer->Clip()->Name().String(), entry->useCounter);
		if (entry->useCounter <= 0) {
			// this entry has not been used for 25 frames,
			// release renderer and remove it
			if (entry->renderer->Release()) {
//printf("deleted ClipRenderer\n");	
			} else {
//printf("released ClipRenderer, but it still has references\n");	
			}
			iterator.Remove();
		}
	}
}



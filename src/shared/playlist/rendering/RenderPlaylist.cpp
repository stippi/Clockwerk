/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RenderPlaylist.h"

#include <new>

#include <stdio.h>

#include <Region.h>

#include "Painter.h"
#include "RenderPlaylistItem.h"
#include "TrackProperties.h"

using std::nothrow;

//compare_playlist_items
static int
compare_playlist_items(const void* a, const void* b)
{
	RenderPlaylistItem* aItem = (RenderPlaylistItem*)*(void**)a;
	RenderPlaylistItem* bItem = (RenderPlaylistItem*)*(void**)b;
	if (aItem->Track() < bItem->Track())
		return 1;
	if (aItem->Track() > bItem->Track())
		return -1;
	return 0;
}

// constructor
RenderPlaylist::RenderPlaylist(const Playlist& other,
		double frame, color_space format, ClipRendererCache* rendererCache)
	: Playlist()
{
	// clone all the needed items at the given frame as RenderPlaylistItems
	// and put the clones in this container's list
	int32 count = other.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = other.ItemAtFast(i);
		if (!other.IsTrackEnabled(item->Track())
			|| !item->HasVideo() || item->IsVideoMuted()
			|| item->StartFrame() > frame
			|| item->EndFrame() < floor(frame)) {
			continue;
		}

		PlaylistItem* clone
			= new (nothrow) RenderPlaylistItem(item,
				frame, format, rendererCache);
		if (clone && !AddItem(clone)) {
			// no memory to put the clone in the list
			printf("RenderPlaylist() - no mem for AddItem()!\n");
			delete clone;
			break;
		}
	}

	SortItems(compare_playlist_items);
}

// destructor
RenderPlaylist::~RenderPlaylist()
{
}

// Generate
status_t
RenderPlaylist::Generate(Painter* painter, double frame)
{
	bool somethingGenerated = false;

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		RenderPlaylistItem* item
			= (RenderPlaylistItem*)ItemAtFast(i);
		// configure painter
		if (!painter->PushState())
			break;
		painter->SetTransformation(item->Transformation());
		painter->SetAlpha(item->Alpha() * 255.0);
		// generate
		if (item->Generate(painter, frame))
			somethingGenerated = true;
		painter->PopState();
	}

	return somethingGenerated ? B_OK : B_ERROR;
}

// RemoveSolidRegion
void
RenderPlaylist::RemoveSolidRegion(BRegion* cleanBG,
								  Painter* painter, double frame)
{
	// remove all areas from the region that are covered by
	// graphical items without any transparency (areas that
	// don't need to be cleaned before rendering)

	// if the graphics state of the painter is already
	// semi-transparent, no item will be solid, so there
	// is no area to be removed (probably means we are
	// a sub-playlist)
	if (painter->GlobalAlpha() < 255)
		return;

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		RenderPlaylistItem* item
			= (RenderPlaylistItem*)ItemAtFast(i);
		// ignore items with transparency, they are not solid
		if (item->Alpha() < 1.0)
			continue;
		// configure painter
		if (!painter->PushState())
			break;
		painter->SetTransformation(item->Transformation());
		// let the item remove it's solid regions
		item->RemoveSolidRegion(cleanBG, painter, frame);

		painter->PopState();
	}
}


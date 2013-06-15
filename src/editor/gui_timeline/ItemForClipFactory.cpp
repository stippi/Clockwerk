/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ItemForClipFactory.h"

#include <new>

#include "ClipPlaylistItem.h"

using std::nothrow;

ItemForClipFactory* gItemForClipFactory = new ItemForClipFactory();

// constructor
ItemForClipFactory::ItemForClipFactory()
{
}

// destructor
ItemForClipFactory::~ItemForClipFactory()
{
}

// PlaylistItemForClip
PlaylistItem*
ItemForClipFactory::PlaylistItemForClip(Clip* clip,
										int64 startFrame,
										uint32 track) const
{
	return new ClipPlaylistItem(clip, startFrame, track);
}

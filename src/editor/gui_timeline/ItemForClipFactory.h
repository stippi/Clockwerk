/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ITEM_FOR_CLIP_FACTORY_H
#define ITEM_FOR_CLIP_FACTORY_H

#include <SupportDefs.h>

class Clip;
class PlaylistItem;

class ItemForClipFactory {
 public:
								ItemForClipFactory();
	virtual						~ItemForClipFactory();

	virtual	PlaylistItem*		PlaylistItemForClip(Clip* clip,
									int64 startFrame = 0,
									uint32 track = 0) const;
};

extern ItemForClipFactory* gItemForClipFactory;

#endif // ITEM_FOR_CLIP_FACTORY_H

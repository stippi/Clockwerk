/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SequenceContainerPlaylist.h"

#include <stdio.h>

#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "PlaylistItem.h"
#include "PropertyAnimator.h"


// constructor 
SequenceContainerPlaylist::SequenceContainerPlaylist()
	: Playlist("SequenceContainerPlaylist")
	, fItemLayoutValid(false)
{
}

// constructor 
SequenceContainerPlaylist::SequenceContainerPlaylist(
		const SequenceContainerPlaylist& other)
	: Playlist(other, true)
	, fItemLayoutValid(other.fItemLayoutValid)
{
}

// destructor
SequenceContainerPlaylist::~SequenceContainerPlaylist()
{
}

// #pragma mark -

//compare_playlist_items
static int
compare_playlist_items(const void* a, const void* b)
{
	PlaylistItem* aItem = (PlaylistItem*)*(void**)a;
	PlaylistItem* bItem = (PlaylistItem*)*(void**)b;
	if (aItem->StartFrame() > bItem->StartFrame())
		return 1;
	if (aItem->StartFrame() < bItem->StartFrame())
		return -1;
	return 0;
}

// ResolveDependencies
status_t
SequenceContainerPlaylist::ResolveDependencies(
	const ServerObjectManager* library)
{
	fItemLayoutValid = false;
	return Playlist::ResolveDependencies(library);
}

// ValidateItemLayout
void
SequenceContainerPlaylist::ValidateItemLayout()
{
	if (fItemLayoutValid)
		return;
	fItemLayoutValid = true;

	// sort the items according to their startframe
	int32 count = CountItems();
	BList items(count);
	for (int32 i = 0; i < count; i++) {
		if (!items.AddItem(ItemAtFast(i)))
			return;
	}
	items.SortItems(compare_playlist_items);

	int64 startFrame = 0;
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = (PlaylistItem*)items.ItemAtFast(i);
		uint64 duration = item->HasMaxDuration() ?
			item->MaxDuration() : item->Duration();
		item->SetDuration(duration);
		item->SetStartFrame(startFrame);
		startFrame += duration;
	}

	Playlist::_ItemsChanged();
}

// _ItemsChanged
void
SequenceContainerPlaylist::_ItemsChanged()
{
	Playlist::_ItemsChanged();

	if (IsInNotificationBlock())
		return;

	fItemLayoutValid = false;
	ValidateItemLayout();
}



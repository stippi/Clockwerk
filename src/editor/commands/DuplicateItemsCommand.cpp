/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DuplicateItemsCommand.h"

#include <new>
#include <stdio.h>

#include <List.h>

#include "HashMap.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "Selection.h"

using std::nothrow;

// constructor
DuplicateItemsCommand::DuplicateItemsCommand(Playlist* list,
											 const PlaylistItem** items,
											 int32 count,
											 Selection* selection)
	: Command(),
	  fPlaylist(list),
	  fSelection(selection),
	  fInfos(NULL),
	  fCount(count),

	  fItemsInserted(false)
{
	if (!fPlaylist || !items || fCount <= 0)
		return;

	fInfos = new (nothrow) PushBackInfo[fCount];
	if (!fInfos)
		return;

	// init the infos
	for (int32 i = 0; i < fCount; i++) {
		if (items[i] && (i > 0 ? fInfos[0].item != NULL : true)) {
			fInfos[i].item = items[i]->Clone(true);
			if (!fInfos[i].item) {
				// delete the first cloned item to
				// indicate failure later on
				delete fInfos[0].item;
				fInfos[0].item = NULL;
			}
		} else {
			fInfos[i].item = NULL;
		}
		if (fInfos[i].item)
			fInfos[i].insert_frame = fInfos[i].item->StartFrame();
		else
			fInfos[i].insert_frame = 0;
		fInfos[i].push_back_start = 0;
		fInfos[i].push_back_frames = 0;
	}
}

// destructor
DuplicateItemsCommand::~DuplicateItemsCommand()
{
	if (!fInfos)
		return;
	for (int32 i = 0; i < fCount; i++) {
		if (!fItemsInserted)
			// the Playlistitems belong to us
			delete fInfos[i].item;
	}
	delete[] fInfos;
}

// InitCheck
status_t
DuplicateItemsCommand::InitCheck()
{
	if (fPlaylist && fInfos && fCount > 0 && fInfos[0].item)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
DuplicateItemsCommand::Perform()
{
	typedef HashKey32<int32> Offset;
	HashMap<Offset, int64> trackOffsets;

	AutoNotificationSuspender _(fPlaylist);

	// insert cloned items
	for (int32 i = 0; i < fCount; i++) {
		PlaylistItem* item = fInfos[i].item;
		if (!item)
			continue;

		uint32 track = item->Track();
		int64 offset = 0;
		if (trackOffsets.ContainsKey(track)) {
			offset += trackOffsets.Get(track);
		} else {
			trackOffsets.Put(track, offset);
		}

		int64 insertFrame = fInfos[i].insert_frame + offset;
		// make room on the playlist
		fPlaylist->MakeRoom(insertFrame, insertFrame + item->Duration(),
							item->Track(), NULL,
							&fInfos[i].push_back_start,
							&fInfos[i].push_back_frames);

		// move the item to the correct location
		item->SetStartFrame(insertFrame);
		// add it
		if (!fPlaylist->AddItem(item)) {
			// ERROR - roll back, remove the items
			// we already added
			fprintf(stderr, "DuplicateItemsCommand::Perform() - "
							"no memory to add items to playlist!\n");
			for (int32 j = i - 1; j >= 0; j--)
				fPlaylist->RemoveItem(fInfos[j].item);
			return B_NO_MEMORY;
		} else {
			// select item
			if (fSelection)
				fSelection->Select(item, i > 0);
		}
	}
	fItemsInserted = true;

	return B_OK;
}

// Undo
status_t
DuplicateItemsCommand::Undo()
{
	// remove items
	for (int32 i = fCount - 1; i >= 0; i--) {
		if (!fInfos[i].item)
			continue;
		// deselect item
		if (fSelection)
			fSelection->Deselect(fInfos[i].item);
		// remove item
		fPlaylist->RemoveItem(fInfos[i].item);
		// undo room making
		fPlaylist->MoveItems(fInfos[i].push_back_start,
							 -fInfos[i].push_back_frames,
							 fInfos[i].item->Track(), NULL);
	}
	fItemsInserted = false;
	return B_OK;
}

// GetName
void
DuplicateItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Duplicate Items";
	else
		name << "Duplicate Item";
}

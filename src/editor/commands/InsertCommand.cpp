/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InsertCommand.h"

#include <new>
#include <stdio.h>

#include "Playlist.h"
#include "PlaylistItem.h"
#include "Selection.h"

using std::nothrow;

// constructor
InsertCommand::InsertCommand(Playlist* list, Selection* selection,
							 PlaylistItem** const items,
							 int32 count,
							 int64 insertFrame,
							 uint32 insertTrack,
							 int32 insertIndex)
	: Command(),
	  fPlaylist(list),
	  fSelection(selection),
	  fItems((items && count > 0) ? new (nothrow) PlaylistItem*[count]
	  							  : NULL),
	  fCount(count),
	  fInsertIndex(insertIndex),

	  fInsertFrame(insertFrame),
	  fInsertTrack(insertTrack),

	  fItemsInserted(false),

	  fPushedBackStart(0),
	  fPushedBackFrames(0)
{
	if (fItems) {
		memcpy(fItems, items, fCount * sizeof(PlaylistItem*));
	}
}

// destructor
InsertCommand::~InsertCommand()
{
	if (!fItemsInserted) {
		// the Playlistitems belong to us
		for (int32 i = 0; i < fCount; i++)
			delete fItems[i];
	}
	delete[] fItems;
}

// InitCheck
status_t
InsertCommand::InitCheck()
{
	if (fPlaylist && fItems && fCount > 0)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
InsertCommand::Perform()
{
	// add up duration of all inserted items
	int64 duration = 0;
	for (int32 i = 0; i < fCount; i++)
		duration += fItems[i]->Duration();

	PlaylistNotificationBlock _(fPlaylist);

	// make room on the playlist
	fPlaylist->MakeRoom(fInsertFrame, fInsertFrame + duration,
						fInsertTrack, NULL,
						&fPushedBackStart, &fPushedBackFrames);

	int32 insertIndex = fInsertIndex;
	if (insertIndex < 0)
		insertIndex = fPlaylist->CountItems();

	// insert items
	int64 insertFrame = fInsertFrame;
	for (int32 i = 0; i < fCount; i++) {
		// make sure the item is at the right place
		fItems[i]->SetStartFrame(insertFrame);
		fItems[i]->SetTrack(fInsertTrack);
		// add it
		if (!fPlaylist->AddItem(fItems[i], insertIndex++)) {
			// ERROR - roll back, remove the items
			// we already added
			fprintf(stderr, "InsertCommand::Perform() - "
							"no memory to add items to playlist!\n");
			for (int32 j = i - 1; j >= 0; j--)
				fPlaylist->RemoveItem(fItems[j]);
			return B_NO_MEMORY;
		}
		insertFrame += fItems[i]->Duration();

		if (fSelection)
			fSelection->Select(fItems[i], i > 0);
	}
	fItemsInserted = true;

	return B_OK;
}

// Undo
status_t
InsertCommand::Undo()
{
	PlaylistNotificationBlock _(fPlaylist);

	// remove items
	for (int32 i = fCount - 1; i >= 0; i--) {
		if (fSelection)
			fSelection->Deselect(fItems[i]);

		fPlaylist->RemoveItem(fItems[i]);
	}
	fItemsInserted = false;
	// undo room making
	fPlaylist->MoveItems(fPushedBackStart, -fPushedBackFrames,
						 fInsertTrack, NULL);
	return B_OK;
}

// GetName
void
InsertCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Insert Items";
	else
		name << "Insert Item";
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DeleteCommand.h"

#include <new>
#include <string.h>

#include "Playlist.h"
#include "PlaylistItem.h"
#include "Selection.h"

using std::nothrow;

// constructor
DeleteCommand::DeleteCommand(Playlist* list,
							 PlaylistItem** items,
							 int32 itemCount,
							 Selection* selection)
	: Command(),
	  fPlaylist(list),
	  fItems(items),
	  fIndices(itemCount > 0 ? new (nothrow) int32[itemCount] : NULL),
	  fItemCount(itemCount),
	  fItemsRemoved(false),
	  fSelection(selection)
{
}

// constructor
DeleteCommand::DeleteCommand(Playlist* list,
							 const PlaylistItem** items,
							 int32 itemCount)
	: Command(),
	  fPlaylist(list),
	  fItems(items && itemCount > 0 ? new (nothrow) PlaylistItem*[itemCount]
	  								: NULL),
	  fIndices(itemCount > 0 ? new (nothrow) int32[itemCount] : NULL),
	  fItemCount(itemCount),
	  fItemsRemoved(false),
	  fSelection(NULL)
{
	if (fItems)
		memcpy(fItems, items, itemCount * sizeof(PlaylistItem*));
}

// destructor
DeleteCommand::~DeleteCommand()
{
	if (fItemsRemoved) {
		// we own the items
		for (int32 i = 0; i < fItemCount; i++)
			delete fItems[i];
	}
	delete[] fItems;
	delete[] fIndices;
}

// InitCheck
status_t
DeleteCommand::InitCheck()
{
	if (fPlaylist && fItems && fIndices)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
DeleteCommand::Perform()
{
	PlaylistNotificationBlock _(fPlaylist);

	for (int32 i = 0; i < fItemCount; i++) {
		if (fSelection)
			fSelection->Deselect(fItems[i]);
		fIndices[i] = fPlaylist->IndexOf(fItems[i]);
		fPlaylist->RemoveItem(fIndices[i]);
	}

	fItemsRemoved = true;

	return B_OK;
}

// Undo
status_t
DeleteCommand::Undo()
{
	status_t ret = B_OK;

	PlaylistNotificationBlock _(fPlaylist);

	for (int32 i = fItemCount - 1; i >= 0; i--) {
		if (fSelection)
			fSelection->Select(fItems[i], i != 0);
		if (!fPlaylist->AddItem(fItems[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}
	fItemsRemoved = false;

	return ret;
}

// GetName
void
DeleteCommand::GetName(BString& name)
{
	if (fItemCount > 1)
		name << "Delete Clips";
	else
		name << "Delete Clip";
}


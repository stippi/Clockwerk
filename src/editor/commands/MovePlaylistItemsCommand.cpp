/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "MovePlaylistItemsCommand.h"

#include <new>
#include <stdio.h>

#include "Playlist.h"
#include "PlaylistItem.h"

using std::nothrow;

// constructor
MovePlaylistItemsCommand::MovePlaylistItemsCommand(
									Playlist* container,
									PlaylistItem** items,
									int32 count,
									int32 toIndex)
	: Command(),
	  fPlaylist(container),
	  fItems(count > 0 ? new (nothrow) PlaylistItem*[count] : NULL),
	  fIndices(count > 0 ? new (nothrow) int32[count] : NULL),
	  fToIndex(toIndex),
	  fCount(count)
{
	if (!fPlaylist || !fItems || !fIndices || !items)
		return;

	memcpy(fItems, items, count * sizeof(PlaylistItem*));

	// init original style indices and
	// adjust toIndex compensating for items that
	// are removed before that index
	int32 itemsBeforeIndex = 0;
	for (int32 i = 0; i < fCount; i++) {
		fIndices[i] = fPlaylist->IndexOf(fItems[i]);
		if (fIndices[i] >= 0 && fIndices[i] < fToIndex)
			itemsBeforeIndex++;
	}
	fToIndex -= itemsBeforeIndex;
}

// destructor
MovePlaylistItemsCommand::~MovePlaylistItemsCommand()
{
	delete[] fItems;
	delete[] fIndices;
}

// InitCheck
status_t
MovePlaylistItemsCommand::InitCheck()
{
	if (!fPlaylist || !fItems || !fIndices)
		return B_NO_INIT;

	// analyse the move, don't return B_OK in case
	// the container state does not change...

	int32 index = fIndices[0];
		// NOTE: fIndices == NULL if fCount < 1

	if (index != fToIndex) {
		// a change is guaranteed
		return B_OK;
	}

	// the insertion index is the same as the index of the first
	// moved item, a change only occures if the indices of the
	// moved items is not contiguous
	bool isContiguous = true;
	for (int32 i = 1; i < fCount; i++) {
		if (fIndices[i] != index + 1) {
			isContiguous = false;
			break;
		}
		index = fIndices[i];
	}
	if (isContiguous) {
		// the container state will not change because of the move
		return B_ERROR;
	}

	return B_OK;
}

// Perform
status_t
MovePlaylistItemsCommand::Perform()
{
	status_t ret = B_OK;

	PlaylistNotificationBlock _(fPlaylist);

	// remove styles from container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fPlaylist->RemoveItem(fItems[i])) {
			ret = B_ERROR;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// add styles to container at the insertion index
	int32 index = fToIndex;
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fPlaylist->AddItem(fItems[i], index++)) {
			ret = B_ERROR;
			break;
		}
	}

	return ret;
}

// Undo
status_t
MovePlaylistItemsCommand::Undo()
{
	status_t ret = B_OK;

	PlaylistNotificationBlock _(fPlaylist);

	// remove styles from container
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fPlaylist->RemoveItem(fItems[i])) {
			ret = B_ERROR;
			break;
		}
	}
	if (ret < B_OK)
		return ret;

	// add styles to container at remembered indices
	for (int32 i = 0; i < fCount; i++) {
		if (fItems[i] && !fPlaylist->AddItem(fItems[i], fIndices[i])) {
			ret = B_ERROR;
			break;
		}
	}

	return ret;
}

// GetName
void
MovePlaylistItemsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << "Move Items";
	else
		name << "Move Item";
}
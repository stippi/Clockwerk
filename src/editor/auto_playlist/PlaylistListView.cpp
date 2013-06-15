/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistListView.h"

#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <String.h>
#include <Window.h>

#include "CommandStack.h"
#include "DeleteCommand.h"
#include "InsertCommand.h"
#include "ItemForClipFactory.h"
#include "MovePlaylistItemsCommand.h"
#include "Observer.h"
#include "PlaylistItem.h"
#include "PlaylistItemItem.h"
#include "RWLocker.h"
#include "Selection.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "TimelineMessages.h"

enum {
	MSG_DRAG_ITEM	= 'drgi',
};

// constructor
PlaylistListView::PlaylistListView(RWLocker* locker, const char* name)
	: SimpleListView(BRect(0.0, 0.0, 20.0, 20.0), name,
					 NULL, B_MULTIPLE_SELECTION_LIST),
	  fPlaylist(NULL),
	  fCommandStack(NULL),
	  fSelection(NULL),
	  fLocker(locker)
{
	SetDragCommand(MSG_DRAG_ITEM);
}

// destructor
PlaylistListView::~PlaylistListView()
{
	_MakeEmpty();

	SetPlaylist(NULL);
}

// SelectionChanged
void
PlaylistListView::SelectionChanged()
{
	// modify global Selection
	if (!fSelection)
		return;

	AutoNotificationSuspender _(fSelection);

	fSelection->DeselectAll();

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItemItem* item
			= dynamic_cast<PlaylistItemItem*>(ItemAt(i));
		if (item->IsSelected())
			fSelection->Select(item->item, true);
	}
}

// MessageReceived
void
PlaylistListView::MessageReceived(BMessage* message)
{
	if (message->what == B_SIMPLE_DATA) {
		// drag and drop, most likely from Tracker
		// TODO: auto insert the clip into this listview
		// as well...
		be_app->PostMessage(message);
	} else {
		if (message->what == MSG_DRAG_CLIP) {
			if (!fPlaylist || !fCommandStack)
				return;

			// inspect the message to retrieve the clips
			ServerObjectManager* library;
			if (message->FindPointer("library", (void**)&library) != B_OK)
				return;
		
			if (!library || !library->ReadLock())
				return;
	
			// temporary list to hold the created items
			BList items;
	
			Clip* clip;
			for (int32 i = 0; message->FindPointer("clip", i,
												   (void**)&clip) == B_OK; i++) {
				if (!library->HasObject(clip)) {
					// the message has arrived asynchronously,
					// so the clip pointer might be stale
					continue;
				}
	
				PlaylistItem* item = gItemForClipFactory->PlaylistItemForClip(clip);
	
				if (!items.AddItem(item)) {
					fprintf(stderr, "InsertClipsDropState::HandleDropMessage() "
									"no memory to insert item in list\n");
					delete item;
					break;
				}
			}

			library->ReadUnlock();
	
			::Command* command = new (std::nothrow) InsertCommand(
				fPlaylist, fSelection, (PlaylistItem**)items.Items(),
				items.CountItems(), 0, 0, _ConvertToPlaylistIndex(fDropIndex));

			AutoWriteLocker locker(fLocker);
			fCommandStack->Perform(command);
		} else
			SimpleListView::MessageReceived(message);
	}
}

// MakeDragMessage
void
PlaylistListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("library", fPlaylist);
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItemItem* item
			= dynamic_cast<PlaylistItemItem*>(ItemAt(CurrentSelection(i)));
		if (item) {
			message->AddPointer("item", (void*)item->item);
		} else
			break;
	}
}

// AcceptDragMessage
bool
PlaylistListView::AcceptDragMessage(const BMessage* message) const
{
	return (message->what == MSG_DRAG_CLIP
			|| message->what == MSG_DRAG_ITEM);
}

// MoveItems
void
PlaylistListView::MoveItems(BList& itemItems, int32 toIndex)
{
	AutoWriteLocker locker(fLocker);
	if (!locker.IsLocked())
		return;

	if (!fCommandStack || !fPlaylist)
		return;

	int32 count = itemItems.CountItems();
	PlaylistItem* items[count];

	for (int32 i = 0; i < count; i++) {
		PlaylistItemItem* itemItem
			= dynamic_cast<PlaylistItemItem*>(
					(BListItem*)itemItems.ItemAtFast(i));
		items[i] = itemItem ? itemItem->item : NULL;
	}

	MovePlaylistItemsCommand* command
		= new (std::nothrow) MovePlaylistItemsCommand(fPlaylist,
							items, count, _ConvertToPlaylistIndex(toIndex));
	if (!command)
		return;

	fCommandStack->Perform(command);
}

// CopyItems
void
PlaylistListView::CopyItems(BList& itemItems, int32 toIndex)
{
	AutoWriteLocker locker(fLocker);
	if (!locker.IsLocked())
		return;

	if (!fCommandStack || !fPlaylist)
		return;

	int32 count = itemItems.CountItems();
	PlaylistItem* items[count];

	for (int32 i = 0; i < count; i++) {
		PlaylistItemItem* itemItem
			= dynamic_cast<PlaylistItemItem*>(
					(BListItem*)itemItems.ItemAtFast(i));
		items[i] = itemItem ? itemItem->item->Clone(true) : NULL;
	}

	InsertCommand* command
		= new (std::nothrow) InsertCommand(fPlaylist, fSelection, items, count,
			0, 0, _ConvertToPlaylistIndex(toIndex));

	if (!command) {
		for (int32 i = 0; i < count; i++)
			delete items[i];
		return;
	}

	fCommandStack->Perform(command);
}

// RemoveItemList
void
PlaylistListView::RemoveItemList(BList& itemItems)
{
	AutoWriteLocker locker(fLocker);
	if (!locker.IsLocked())
		return;

	if (!fCommandStack || !fPlaylist)
		return;

	int32 count = itemItems.CountItems();
	// TODO: adjust DeleteCommand to take "PlaylistItem** const"
	PlaylistItem** items = new (std::nothrow) PlaylistItem*[count];
	if (!items)
		return;

	for (int32 i = 0; i < count; i++) {
		PlaylistItemItem* itemItem
			= dynamic_cast<PlaylistItemItem*>(
					(BListItem*)itemItems.ItemAtFast(i));
		items[i] = itemItem ? itemItem->item : NULL;
	}

	DeleteCommand* command
		= new (std::nothrow) DeleteCommand(fPlaylist, items, count, NULL);
	fCommandStack->Perform(command);
}

// CloneItem
BListItem*
PlaylistListView::CloneItem(int32 index) const
{
	if (PlaylistItemItem* item
			= dynamic_cast<PlaylistItemItem*>(ItemAt(index))) {
		return new PlaylistItemItem(item->item,
							  const_cast<PlaylistListView*>(this));
	}
	return NULL;
}

// #pragma mark -

// PlaylistItemAdded
void
PlaylistListView::ItemAdded(Playlist* playlist, PlaylistItem* item, int32 index)
{
	// NOTE: we are in the thread that messed with the
	// ServerObjectManager, so no need to lock the
	// manager, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	_AddItem(item, index);

	UnlockLooper();
}

// PlaylistItemRemoved
void
PlaylistListView::ItemRemoved(Playlist* playlist, PlaylistItem* item)
{
	// NOTE: we are in the thread that messed with the
	// ServerObjectManager, so no need to lock the
	// manager, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	_RemoveItem(item);

	UnlockLooper();
}

// DurationChanged
void
PlaylistListView::DurationChanged(Playlist* playlist, uint64 duration)
{
	// TODO: apply arranger here?
}

// #pragma mark -

// ObjectChanged
void
PlaylistListView::ObjectChanged(const Observable* object)
{
	PlaylistItem* item = const_cast<PlaylistItem*>(
							dynamic_cast<const PlaylistItem*>(object));
	if (!item)
		return;

	bool needsManaged = item->Track() <= 1;
	PlaylistItemItem* itemItem = _ItemForItem(item);

	if (needsManaged == (itemItem != NULL))
		return;
	else if (needsManaged)
		_AddManagedItem(item, _ConvertToManagedIndex(item));
	else
		_RemoveManagedItem(item);
}

// #pragma mark -

// SetPlaylist
void
PlaylistListView::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	// detach from old library
	if (fPlaylist) {
		fPlaylist->RemoveListObserver(this);
		int32 count = fPlaylist->CountItems();
		for (int32 i = 0; i < count; i++) {
			PlaylistItem* item = fPlaylist->ItemAtFast(i);
			_RemoveItem(item);
		}
	}

	_MakeEmpty();

	fPlaylist = playlist;

	if (!fPlaylist)
		return;

	fPlaylist->AddListObserver(this);

	// sync
	AutoReadLocker locker(fLocker);

	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = fPlaylist->ItemAtFast(i);
		_AddItem(item, i);
	}
}

// SetSelection
void
PlaylistListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// SetObjectLibrary
void
PlaylistListView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// #pragma mark -

// _AddItem
bool
PlaylistListView::_AddItem(PlaylistItem* item, int32 index)
{
	if (!item)
		return false;

	item->AddObserver(this);

	if (item->Track() <= 1)
		return _AddManagedItem(item, _ConvertToManagedIndex(index));

	return true;
}

// _AddManagedItem
bool
PlaylistListView::_AddManagedItem(PlaylistItem* item, int32 index)
{
	return AddItem(new PlaylistItemItem(item, this), index);
}

// _RemoveItem
bool
PlaylistListView::_RemoveItem(PlaylistItem* item)
{
	item->RemoveObserver(this);

	if (HasItem(_ItemForItem(item)))
		return _RemoveManagedItem(item);

	return true;
}

// _RemoveManagedItem
bool
PlaylistListView::_RemoveManagedItem(PlaylistItem* item)
{
	PlaylistItemItem* itemItem = _ItemForItem(item);
	if (itemItem && RemoveItem(itemItem)) {
		delete itemItem;
		return true;
	}
	return false;
}

// _ConvertToManagedIndex
int32
PlaylistListView::_ConvertToManagedIndex(PlaylistItem* item) const
{
	return _ConvertToManagedIndex(fPlaylist->IndexOf(item));
}

// _ConvertToManagedIndex
int32
PlaylistListView::_ConvertToManagedIndex(int32 index) const
{
	if (index < 0)
		return -1;

	for (int32 i = index - 1; i >= 0; i--) {
		PlaylistItem* item = fPlaylist->ItemAt(i);
		int32 managedIndex = _IndexOf(item);
		if (managedIndex >= 0)
			return managedIndex + 1;
	}
	return 0;
}

// _ConvertToPlaylistIndex
int32
PlaylistListView::_ConvertToPlaylistIndex(int32 index) const
{
	if (index == CountItems())
		return fPlaylist->CountItems();

	PlaylistItemItem* itemItem = dynamic_cast<PlaylistItemItem*>(ItemAt(index));
	if (!itemItem)
		return -1;

	return fPlaylist->IndexOf(itemItem->item);
}

// #pragma mark -

// _IndexOf
int32
PlaylistListView::_IndexOf(PlaylistItem* item) const
{
	for (int32 i = 0; PlaylistItemItem* itemItem
			= dynamic_cast<PlaylistItemItem*>(ItemAt(i)); i++) {
		if (itemItem->item == item)
			return i;
	}
	return -1;
}

// _ItemForItem
PlaylistItemItem*
PlaylistListView::_ItemForItem(PlaylistItem* item) const
{
	return dynamic_cast<PlaylistItemItem*>(ItemAt(_IndexOf(item)));
}

// _MakeEmpty
void
PlaylistListView::_MakeEmpty()
{
	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		delete RemoveItem(i);
	}
}


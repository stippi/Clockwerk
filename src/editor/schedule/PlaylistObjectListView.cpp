/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistObjectListView.h"

#include <stdio.h>

#include <Message.h>
#include <Messenger.h>

#include "support_date.h"

#include "Column.h"
#include "ColumnItem.h"
#include "CommonPropertyIDs.h"
#include "ObjectCLVItem.h"
#include "Playlist.h"

enum {
	MSG_PLAYLIST_PROPERTIES_CHANGED	= 'plpc',
};

// constructor
PlaylistObjectListView::PlaylistObjectListView(const char* name,
		BMessage* message, BHandler* target)
	: ObjectColumnListView(name, message, target)
	, fDurationColumnIndex(CountColumns())
	, fPlaylistObserver(this)
{
	AddColumn(new Column("Duration", "duration", 50.0,
		COLUMN_VISIBLE | COLUMN_SORT_KEYABLE), fDurationColumnIndex);

	fPlaylistObserver.SetNotificationTypes(NOTIFY_DURATION_CHANGES);
}

// destructor
PlaylistObjectListView::~PlaylistObjectListView()
{
}

// MessageReceived
void
PlaylistObjectListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PLAYLIST_PROPERTIES_CHANGED:
			Sync();
			break;
		case MSG_PLAYLIST_DURATION_CHANGED: {
			Playlist* playlist;
			if (message->FindPointer("playlist", (void**)&playlist) < B_OK)
				break;
			AutoReadLocker locker(Locker());
			ObjectCLVItem* item = ItemForObject(playlist);
			if (item && UpdateItem(item)) {
				if (IsSortingColumn(fDurationColumnIndex))
					Sort();
				else
					InvalidateItem(item);
			}
			break;
		}
		default:
			ObjectColumnListView::MessageReceived(message);
			break;
	}
}

// ObjectAdded
void
PlaylistObjectListView::ObjectAdded(ServerObject* object, int32 index)
{
	if (Playlist* pl = dynamic_cast<Playlist*>(object)) {
		pl->Acquire();
		pl->AddObserver(this);
		pl->AddListObserver(&fPlaylistObserver);
	}
	ObjectColumnListView::ObjectAdded(object, index);
}

// ObjectRemoved
void
PlaylistObjectListView::ObjectRemoved(ServerObject* object)
{
	if (Playlist* pl = dynamic_cast<Playlist*>(object)) {
		pl->RemoveObserver(this);
		pl->RemoveListObserver(&fPlaylistObserver);
		pl->Release();
	}
	ObjectColumnListView::ObjectRemoved(object);
}

// AcceptObject
bool
PlaylistObjectListView::AcceptObject(ServerObject* object)
{
	Playlist* pl = dynamic_cast<Playlist*>(object);
	if (pl != NULL) {
		return pl->Value(PROPERTY_PLAYLIST_SCHEDULEABLE, false);
	}
	return false;
}

// UpdateItem
bool
PlaylistObjectListView::UpdateItem(ObjectCLVItem* item) const
{
	if (!item)
		return false;

	bool invalidate = ObjectColumnListView::UpdateItem(item);

	Playlist* playlist = dynamic_cast<Playlist*>(item->object);
	if (!playlist)
		return invalidate;

	TextColumnItem* columnItem = dynamic_cast<TextColumnItem*>(
		item->ColumnItemAt(fDurationColumnIndex));

	BString duration = string_for_frame(playlist->Duration());

	if (!columnItem || duration != columnItem->Text()) {
		item->SetContent(fDurationColumnIndex, duration.String(), false);
		invalidate = true;
	}

	return invalidate;
}


// ObjectChanged
void
PlaylistObjectListView::ObjectChanged(const Observable* object)
{
	BMessenger messenger(this);
	messenger.SendMessage(MSG_PLAYLIST_PROPERTIES_CHANGED, this);
}


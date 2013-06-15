/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <Message.h>

#include "PlaylistLOAdapter.h"

// constructor
PlaylistLOAdapter::PlaylistLOAdapter(BHandler* handler)
	: AbstractLOAdapter(handler)
	, fNotificationTypes(NOTIFY_ALL)
{
}

// constructor
PlaylistLOAdapter::PlaylistLOAdapter(const BMessenger& messenger)
	: AbstractLOAdapter(messenger)
	, fNotificationTypes(NOTIFY_ALL)
{
}

// destructor
PlaylistLOAdapter::~PlaylistLOAdapter()
{
}

// ItemAdded
void
PlaylistLOAdapter::ItemAdded(Playlist* playlist, PlaylistItem* item,
	int32 index)
{
	if (!(fNotificationTypes & NOTIFY_ITEM_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_ITEM_ADDED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddPointer("item", (void*)item);
	message.AddInt32("index", index);
	DeliverMessage(message);
}

// ItemRemoved
void
PlaylistLOAdapter::ItemRemoved(Playlist* playlist, PlaylistItem* item)
{
	if (!(fNotificationTypes & NOTIFY_ITEM_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_ITEM_REMOVED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddPointer("item", (void*)item);
	DeliverMessage(message);
}

// DurationChanged
void
PlaylistLOAdapter::DurationChanged(Playlist* playlist, uint64 duration)
{
	if (!(fNotificationTypes & NOTIFY_DURATION_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_DURATION_CHANGED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddInt64("duration", duration);
	DeliverMessage(message);
}

// TrackPropertiesChanged
void
PlaylistLOAdapter::TrackPropertiesChanged(Playlist* playlist, uint32 track)
{
	if (!(fNotificationTypes & NOTIFY_TRACK_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_TRACK_PROPERTIES_CHANGED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddInt32("track", track);
	DeliverMessage(message);
}

// TrackMoved
void
PlaylistLOAdapter::TrackMoved(Playlist* playlist, uint32 oldIndex,
	uint32 newIndex)
{
	if (!(fNotificationTypes & NOTIFY_TRACK_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_TRACK_MOVED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddInt32("old index", oldIndex);
	message.AddInt32("new index", newIndex);
	DeliverMessage(message);
}

// TrackInserted
void
PlaylistLOAdapter::TrackInserted(Playlist* playlist, uint32 track)
{
	if (!(fNotificationTypes & NOTIFY_TRACK_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_TRACK_INSERTED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddInt32("track", track);
	DeliverMessage(message);
}

// TrackRemoved
void
PlaylistLOAdapter::TrackRemoved(Playlist* playlist, uint32 track)
{
	if (!(fNotificationTypes & NOTIFY_TRACK_CHANGES))
		return;

	BMessage message(MSG_PLAYLIST_TRACK_REMOVED);
	message.AddPointer("playlist", (void*)playlist);
	message.AddInt32("track", track);
	DeliverMessage(message);
}

// #pragma mark -

// NotificationBlockStarted
void
PlaylistLOAdapter::NotificationBlockStarted(Playlist* playlist)
{
	if (!(fNotificationTypes & NOTIFY_BLOCKS))
		return;

	BMessage message(MSG_PLAYLIST_NOTIFICATION_BLOCK_STARTED);
	message.AddPointer("playlist", (void*)playlist);
	DeliverMessage(message);
}

// NotificationBlockFinished
void
PlaylistLOAdapter::NotificationBlockFinished(Playlist* playlist)
{
	if (!(fNotificationTypes & NOTIFY_BLOCKS))
		return;

	BMessage message(MSG_PLAYLIST_NOTIFICATION_BLOCK_FINISHED);
	message.AddPointer("playlist", (void*)playlist);
	DeliverMessage(message);
}

// #pragma mark -

// SetNotificationTypes
void
PlaylistLOAdapter::SetNotificationTypes(uint32 types)
{
	fNotificationTypes = types;
}


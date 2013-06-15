/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistObserver.h"

// constructor 
PlaylistObserver::PlaylistObserver()
{
}

// destructor
PlaylistObserver::~PlaylistObserver()
{
}

// ItemAdded
void
PlaylistObserver::ItemAdded(Playlist* playlist, PlaylistItem* item,
	int32 index)
{
}

// ItemRemoved
void
PlaylistObserver::ItemRemoved(Playlist* playlist, PlaylistItem* item)
{
}

// DurationChanged
void
PlaylistObserver::DurationChanged(Playlist* playlist, uint64 duration)
{
}

// MaxTrackChanged
void
PlaylistObserver::MaxTrackChanged(Playlist* playlist, uint32 maxTrack)
{
}

// TrackPropertiesChanged
void
PlaylistObserver::TrackPropertiesChanged(Playlist* playlist, uint32 track)
{
}

// TrackMoved
void
PlaylistObserver::TrackMoved(Playlist* playlist, uint32 oldIndex,
	uint32 newIndex)
{
}

// TrackInserted
void
PlaylistObserver::TrackInserted(Playlist* playlist, uint32 track)
{
}

// TrackRemoved
void
PlaylistObserver::TrackRemoved(Playlist* playlist, uint32 track)
{
}

// #pragma mark -

// NotificationBlockStarted
void
PlaylistObserver::NotificationBlockStarted(Playlist* playlist)
{
}

// NotificationBlockFinished
void
PlaylistObserver::NotificationBlockFinished(Playlist* playlist)
{
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_OBSERVER_H
#define PLAYLIST_OBSERVER_H

#include <SupportDefs.h>

class Playlist;
class PlaylistItem;

class PlaylistObserver {
 public:
								PlaylistObserver();
	virtual						~PlaylistObserver();

	virtual	void				ItemAdded(Playlist* playlist,
									PlaylistItem* item, int32 index);
	virtual	void				ItemRemoved(Playlist* playlist,
									PlaylistItem* item);

	virtual	void				DurationChanged(Playlist* playlist,
									uint64 duration);
	virtual	void				MaxTrackChanged(Playlist* playlist,
									uint32 maxTrack);

	virtual	void				TrackPropertiesChanged(Playlist* playlist,
									uint32 track);
	virtual	void				TrackMoved(Playlist* playlist,
									uint32 oldIndex, uint32 newIndex);
	virtual	void				TrackInserted(Playlist* playlist, uint32 track);
	virtual	void				TrackRemoved(Playlist* playlist, uint32 track);

	virtual	void				NotificationBlockStarted(Playlist* playlist);
	virtual	void				NotificationBlockFinished(Playlist* playlist);
};

#endif // PLAYLIST_OBSERVER_H

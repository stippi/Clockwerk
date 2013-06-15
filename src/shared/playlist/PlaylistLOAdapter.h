/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_LO_ADAPTER_H
#define PLAYLIST_LO_ADAPTER_H

#include "AbstractLOAdapter.h"
#include "PlaylistObserver.h"

enum {
	MSG_PLAYLIST_ITEM_ADDED						= 'plia',
	MSG_PLAYLIST_ITEM_REMOVED					= 'plir',
	MSG_PLAYLIST_DURATION_CHANGED				= 'pldc',
	MSG_PLAYLIST_TRACK_PROPERTIES_CHANGED		= 'pltc',
	MSG_PLAYLIST_TRACK_MOVED					= 'pltm',
	MSG_PLAYLIST_TRACK_INSERTED					= 'plti',
	MSG_PLAYLIST_TRACK_REMOVED					= 'pltr',
	MSG_PLAYLIST_NOTIFICATION_BLOCK_STARTED		= 'plns',
	MSG_PLAYLIST_NOTIFICATION_BLOCK_FINISHED	= 'plnf',
};

enum {
	NOTIFY_ITEM_CHANGES							= 0x01,
	NOTIFY_DURATION_CHANGES						= 0x02,
	NOTIFY_TRACK_CHANGES						= 0x04,
	NOTIFY_BLOCKS								= 0x08,

	NOTIFY_ALL									= 0xff
};

class PlaylistLOAdapter : public AbstractLOAdapter,
						  public PlaylistObserver {
 public:
								PlaylistLOAdapter(BHandler* handler);
								PlaylistLOAdapter(
									const BMessenger& messenger);
	virtual						~PlaylistLOAdapter();

	// PlaylistObserver interface
	virtual	void				ItemAdded(Playlist* playlist,
									PlaylistItem* item, int32 index);
	virtual	void				ItemRemoved(Playlist* playlist,
									PlaylistItem* item);

	virtual	void				DurationChanged(Playlist* playlist,
									uint64 duration);

	virtual	void				TrackPropertiesChanged(Playlist* playlist,
									uint32 track);
	virtual	void				TrackMoved(Playlist* playlist, uint32 oldIndex,
									uint32 newIndex);
	virtual	void				TrackInserted(Playlist* playlist, uint32 track);
	virtual	void				TrackRemoved(Playlist* playlist, uint32 track);

	virtual	void				NotificationBlockStarted(Playlist* playlist);
	virtual	void				NotificationBlockFinished(Playlist* playlist);

	// PlaylistLOAdapter
			void				SetNotificationTypes(uint32 types);

 private:
			uint32				fNotificationTypes;
};

#endif	// PLAYLIST_LO_ADAPTER_H

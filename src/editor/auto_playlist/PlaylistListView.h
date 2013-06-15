/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_LIST_VIEW_H
#define PLAYLIST_LIST_VIEW_H

#include "ListViews.h"
#include "Observer.h"
#include "Playlist.h"
#include "PlaylistObserver.h"
#include "RWLocker.h"

class CommandStack;
class PlaylistItemItem;
class Selection;

class PlaylistListView : public SimpleListView,
						 public PlaylistObserver,
						 public Observer {
 public:
								PlaylistListView(
									RWLocker* locker,
									const char* name);
	virtual						~PlaylistListView();

	// SimpleListView interface
	virtual	void				SelectionChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	bool				AcceptDragMessage(const BMessage* message) const;
	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& items);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	// PlaylistObserver interface
	virtual	void				ItemAdded(Playlist* playlist,
									PlaylistItem* item, int32 index);
	virtual	void				ItemRemoved(Playlist* playlist,
									PlaylistItem* item);
	virtual	void				DurationChanged(Playlist* playlist,
									uint64 duration);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// PlaylistListView
			void				SetPlaylist(Playlist* playlist);
			void				SetCommandStack(CommandStack* stack);
			void				SetSelection(Selection* selection);

 private:
			bool				_AddItem(PlaylistItem* item, int32 index);
			bool				_AddManagedItem(PlaylistItem* item, int32 index);
			bool				_RemoveItem(PlaylistItem* item);
			bool				_RemoveManagedItem(PlaylistItem* item);
			int32				_ConvertToManagedIndex(PlaylistItem* item) const;
			int32				_ConvertToManagedIndex(int32 index) const;
			int32				_ConvertToPlaylistIndex(int32 index) const;

			int32				_IndexOf(PlaylistItem* item) const;
			PlaylistItemItem*	_ItemForItem(PlaylistItem* item) const;
			void				_MakeEmpty();

			Playlist*			fPlaylist;
			CommandStack*		fCommandStack;
			Selection*			fSelection;
			RWLocker*			fLocker;
};

#endif // PLAYLIST_LIST_VIEW_H

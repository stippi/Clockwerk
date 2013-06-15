/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_ITEM_ITEM_H
#define PLAYLIST_ITEM_ITEM_H

#include "ListViews.h"
#include "Observer.h"

class PlaylistItem;

class PlaylistItemItem : public SimpleItem, public Observer {
 public:
								PlaylistItemItem(PlaylistItem* item,
									SimpleListView* listView);

	virtual						~PlaylistItemItem();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* item);

	// PlaylistItemItem
			void				SetItem(PlaylistItem* item);

			void				UpdateText();


			PlaylistItem* 		item;

 private:
			SimpleListView*		fListView;
};

#endif // PLAYLIST_ITEM_ITEM_H

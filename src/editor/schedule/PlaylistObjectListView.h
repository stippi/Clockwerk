/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_OBJECT_LIST_VIEW_H
#define PLAYLIST_OBJECT_LIST_VIEW_H

#include "ObjectColumnListView.h"
#include "Observer.h"
#include "PlaylistLOAdapter.h"

class PlaylistObjectListView : public ObjectColumnListView,
							   public Observer {
 public:
								PlaylistObjectListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~PlaylistObjectListView();

	// ObjectListView interface
	virtual	void				MessageReceived(BMessage* message);

	virtual	bool				AcceptObject(ServerObject* object);

 protected:
	virtual	void				ObjectAdded(ServerObject* object,
											int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

	virtual	bool				UpdateItem(ObjectCLVItem* item) const;

 public:
	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

 private:
			int32				fDurationColumnIndex;
			PlaylistLOAdapter	fPlaylistObserver;
};

#endif // PLAYLIST_OBJECT_LIST_VIEW_H

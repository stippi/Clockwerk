/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_LIST_VIEW_H
#define CLIP_LIST_VIEW_H

#include <String.h>

#include "ListViews.h"
#include "Observer.h"
#include "PlaylistObserver.h"
#include "ServerObjectManager.h"

class Clip;
class ClipItemPainter;
class ClipListView;
class Playlist;
class Selection;

class ClipListItem : public SimpleItem,
					 public Observer {
 public:
					ClipListItem(Clip* c,
								 ClipListView* listView);
					ClipListItem(Clip* c,
								 ClipListView* listView,
								 ClipItemPainter* painter);
	virtual			~ClipListItem();

	// SimpleItem interface
	virtual	void	Draw(BView* owner, BRect frame, uint32 flags);
	virtual	void	Update(BView* owner, const BFont* font);

	// Observer interface
	virtual	void	ObjectChanged(const Observable* object);

	// ClipListItem
			void	SetClip(Clip* c);
			void	SetPainter(ClipItemPainter* painter);

			void	UpdateText();
			void	SetRemoved(bool removed);
			bool	Removed() const { return fRemoved; }
			void	Invalidate();

	Clip* 			clip;

 private:
	ClipListView*	fListView;
	ClipItemPainter* fPainter;
	bool			fRemoved;
	uint32			fReloadToken;
};

class ClipItemPainter {
 public:
							ClipItemPainter();
	virtual					~ClipItemPainter();

			void			MakeIcon(Clip* clip);
	virtual	void			PaintItem(BView* owner,
									  ClipListItem* item,
									  BRect itemFrame,
									  uint32 flags);
 private:
			BBitmap*		fIcon;
};

class ClipListView : public SimpleListView,
					 public SOMListener,
					 public PlaylistObserver,
					 public Observer {
 public:
	class ItemSorter {
	 public:
								ItemSorter();
		virtual					~ItemSorter();


				void			SetPlaylistClipsOnly(bool playlistOnly);
				bool			PlaylistClipsOnly() const
									{ return fPlaylistClipsOnly; }
				void			SetNameContainsFilterString(const char* string);
				const BString&	NameContainsFilterString() const
									{ return fNameContainsFilterString; }

		virtual	int32			IndexForClip(ClipListView* listView,
											 Clip* clip) const;
	 private:
	 			bool			fPlaylistClipsOnly;
	 			BString			fNameContainsFilterString;
	};

								ClipListView(const char* name,
											 BMessage* selectionChangeMessage = NULL,
											 BMessage* invokeMessage = NULL,
											 BHandler* target = NULL);
	virtual						~ClipListView();

	// SimpleListView
	virtual	void				SelectionChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	bool				AcceptDragMessage(const BMessage* message) const;
	virtual	void				SetDropTargetRect(const BMessage* message,
												  BPoint where);

	virtual	void				MoveItems(BList& items, int32 toIndex);
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& indices);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	virtual	void				DoubleClicked(int32 index);

	// SOMListener
	virtual	void				ObjectAdded(ServerObject* object, int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

	// PlaylistObserver interface
	virtual	void				ItemAdded(::Playlist* playlist,
									PlaylistItem* item, int32 index);
	virtual	void				ItemRemoved(::Playlist* playlist,
									PlaylistItem* item);

	// Observer
	virtual	void				ObjectChanged(const Observable* object);

	// ClipListView
			void				SetObjectLibrary(ServerObjectManager* library);
			void				SetSelection(Selection* selection);
			void				SetPlaylist(::Playlist* playlist);
			::Playlist*			Playlist() const
									{ return fPlaylist; }

			void				SetItemSorter(ItemSorter* sorter);
			const BString&		NameContainsFilterString() const;

			int32				IndexForClip(Clip* clip) const;

 private:
			bool				_AddClip(Clip* clip, int32 index);
			bool				_RemoveClip(Clip* clip);

			ClipListItem*		_ItemForClip(Clip* clip) const;
			void				_MakeEmpty();
			void				_Sync();

			int32				_FilterClip(Clip* clip);

			BMessage*			fSelectionMessage;
			BMessage*			fInvokeMessage;

			ServerObjectManager* fClipLibrary;
			Selection*			fSelection;
			::Playlist*			fPlaylist;

			ItemSorter*			fSorter;
			int32				fIgnoreSelectionChanged;
};

#endif // CLIP_LIST_VIEW_H

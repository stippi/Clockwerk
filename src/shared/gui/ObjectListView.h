/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef OBJECT_LIST_VIEW_H
#define OBJECT_LIST_VIEW_H

#include "ListViews.h"
#include "ServerObjectManager.h"

class ObjectItem;
class ServerObject;
class Selection;

class ObjectListView : public SimpleListView {
 public:
								ObjectListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~ObjectListView();

	// SimpleListView
	virtual	void				SelectionChanged();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeDragMessage(BMessage* message) const;

	virtual	bool				AcceptDragMessage(const BMessage* message) const;
	virtual	void				CopyItems(BList& items, int32 toIndex);
	virtual	void				RemoveItemList(BList& indices);

	virtual	BListItem*			CloneItem(int32 atIndex) const;

	// ObjectListView
	virtual	void				SetObjectLibrary(ServerObjectManager* library);
			void				SetSelection(Selection* selection);

			void				Sync();

	virtual	bool				AcceptObject(ServerObject* object) = 0;

	virtual	bool				UpdateItem(ObjectItem* item) const;
	virtual	void				SetupDrawFont(BView* view,
									const ObjectItem* item, BFont* font);

 protected:
	virtual	void				ObjectAdded(ServerObject* object, int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

 private:
			bool				_AddObject(ServerObject* object,
									int32 index = -1);
			bool				_RemoveObject(ServerObject* object);

			ObjectItem*			_ItemForObject(ServerObject* object) const;
			void				_MakeEmpty();

			int32				_InsertIndexFor(ServerObject* object) const;

			BMessage*			fMessage;

			ServerObjectManager* fObjectLibrary;
			Selection*			fSelection;
			AsyncSOMListener*	fSOMListener;
};

#endif // OBJECT_LIST_VIEW_H

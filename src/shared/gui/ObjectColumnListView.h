/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef OBJECT_COLUMN_LIST_VIEW_H
#define OBJECT_COLUMN_LIST_VIEW_H

#include "ColumnListView.h"
#include "ServerObjectManager.h"

class ObjectCLVItem;
class ServerObject;
class Selectable;
class Selection;

class ObjectColumnListView : public ColumnListView {
 public:
								ObjectColumnListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~ObjectColumnListView();

	// ColumnListView interface
	virtual	void				SelectionChanged();
	virtual	void				ItemDoubleClicked(int32 index);

	virtual	void				MessageReceived(BMessage* message);

	virtual	bool				MakeDragMessage(BMessage* message,
									Column** column1, Column** column2);

//	virtual	bool				AcceptDragMessage(const BMessage* message) const;

	// ObjectColumnListView
			void				SetDragCommand(uint32 command);

	virtual	void				SetObjectLibrary(ServerObjectManager* library);
			void				SetSelection(Selection* selection);
			RWLocker*			Locker() const
									{ return fObjectLibrary->Locker(); }

			void				Sync();

	virtual	bool				AcceptObject(ServerObject* object) = 0;
	virtual	bool				UpdateItem(ObjectCLVItem* item) const;

			ObjectCLVItem*		ItemForObject(ServerObject* object) const;

			void				StoreSettings(BMessage* archive,
									const char* name) const;
			void				RestoreSettings(BMessage* archive,
									const char* name);

			bool				AddObject(ServerObject* object,
									int32 index = -1);
			bool				RemoveObject(ServerObject* object);

 protected:
	virtual	void				ObjectAdded(ServerObject* object, int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

 private:

			void				_MakeEmpty();

			ObjectCLVItem*		_SendSelectionChangedMessage(int32 index,
									bool invoked);

			BMessage*			fMessage;
			uint32				fDragCommand;

			ServerObjectManager* fObjectLibrary;
			AsyncSOMListener*	fSOMListener;

			Selection*			fSelection;
			Selectable*			fSelectedObject;

			float				fItemHeight;
};

#endif // OBJECT_COLUMN_LIST_VIEW_H

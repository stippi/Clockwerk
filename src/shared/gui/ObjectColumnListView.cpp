/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ObjectColumnListView.h"

#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <String.h>
#include <Window.h>

#include "support_ui.h"

#include "AsyncObserver.h"
#include "Column.h"
#include "ColumnItem.h"
#include "CommonPropertyIDs.h"
#include "ObjectCLVItem.h"
#include "Observer.h"
#include "Selectable.h"
#include "Selection.h"
#include "ServerObject.h"

enum {
	MSG_DRAG_OBJECT	= 'drgo',
};

enum {
	kNameColumnIndex = 0,
};

// constructor
ObjectColumnListView::ObjectColumnListView(const char* name,
		BMessage* message, BHandler* target)
	: ColumnListView()
	, fMessage(message)

	, fObjectLibrary(NULL)
	, fSOMListener(new AsyncSOMListener(this))

	, fSelection(NULL)
	, fSelectedObject(NULL)

	, fItemHeight(plain_font_height())
{
	SetName(name);
	SetSelectionMode(CLV_SINGLE_SELECTION);
	SetTarget(target);
	SetSortCompareFunction(EasyColumnListItem::StandardCompare);

	AddColumn(new Column("Name", "name", 320.0,
		COLUMN_VISIBLE | COLUMN_SORT_KEYABLE), kNameColumnIndex);
}

// destructor
ObjectColumnListView::~ObjectColumnListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fObjectLibrary)
		fObjectLibrary->RemoveListener(fSOMListener);

	delete fSOMListener;
}

// SelectionChanged
void
ObjectColumnListView::SelectionChanged()
{
	// NOTE: this is a single selection list!

	ObjectCLVItem* item = _SendSelectionChangedMessage(
		CurrentSelection(0), false);

	// modify global Selection
	if (!fSelection)
		return;

	if (item) {
		Selectable* selectable = dynamic_cast<Selectable*>(item->object);
		fSelectedObject = selectable;
		fSelection->Select(fSelectedObject);
	} else {
		fSelection->Deselect(fSelectedObject);
		fSelectedObject = NULL;
	}
}

// ItemDoubleClicked
void
ObjectColumnListView::ItemDoubleClicked(int32 index)
{
printf("ItemDoubleClicked\n");
	_SendSelectionChangedMessage(index, true);
}

// MessageReceived
void
ObjectColumnListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
			// drag and drop, most likely from Tracker
			be_app->PostMessage(message);
			break;
		case MSG_DRAG_OBJECT:
			// don't allow drag sorting for now
			break;
		case AsyncObserver::MSG_OBJECT_CHANGED: {
			// handle asynchronous notifications for our items
			Observable* observable;
			if (message->FindPointer("object", (void**)&observable) == B_OK) {
				ServerObject* object = dynamic_cast<ServerObject*>(observable);
				if (object) {
					AutoReadLocker locker(Locker());
					ObjectCLVItem* item = ItemForObject(object);
					if (item && UpdateItem(item)) {
						Sort();
						InvalidateItem(item);
					}
				}
			}
			break;
		}
		case AsyncSOMListener::MSG_OBJECT_ADDED: {
			ServerObject* object;
			int32 index;
			if (message->FindPointer("object", (void**)&object) == B_OK
				&& message->FindInt32("index", &index) == B_OK) {
				ObjectAdded(object, index);
			}
			break;
		}
		case AsyncSOMListener::MSG_OBJECT_REMOVED: {
			ServerObject* object;
			if (message->FindPointer("object", (void**)&object) == B_OK) {
				ObjectRemoved(object);
			}
			break;
		}
		default:
			ColumnListView::MessageReceived(message);
			break;
	}
}

// MakeDragMessage
bool
ObjectColumnListView::MakeDragMessage(BMessage* message,
	Column** column1, Column** column2)
{
	int32 count = CountSelectedItems();
	if (count == 0)
		return false;

	*column1 = ColumnAt(kNameColumnIndex);
	*column2 = NULL;

	message->what = fDragCommand;
	message->AddPointer("library", fObjectLibrary);
	for (int32 i = 0; i < count; i++) {
		ObjectCLVItem* item = dynamic_cast<ObjectCLVItem*>(
			ItemAt(CurrentSelection(i)));
		if (item)
			message->AddPointer("object", (void*)item->object);
	}

//		message->AddInt32("be:actions", B_COPY_TARGET);
//		message->AddInt32("be:actions", B_TRASH_TARGET);
//
//		message->AddString("be:types", B_FILE_MIME_TYPE);
////		message->AddString("be:filetypes", "");
////		message->AddString("be:type_descriptions", "");
//
//		message->AddString("be:clip_name", item->object->Name());
//
//		message->AddString("be:originator", "Clockwerk");
//		message->AddPointer("be:originator_data", (void*)item->object);

	return true;
}

//// AcceptDragMessage
//bool
//ObjectColumnListView::AcceptDragMessage(const BMessage* message) const
//{
//	// don't allow any dropping for now
//	return false;
//}

// #pragma mark -

// SetDragCommand
void
ObjectColumnListView::SetDragCommand(uint32 command)
{
	fDragCommand = command;
}

// SetObjectLibrary
void
ObjectColumnListView::SetObjectLibrary(ServerObjectManager* library)
{
	if (fObjectLibrary == library)
		return;

	// detach from old library
	if (fObjectLibrary) {
		if (fObjectLibrary->WriteLock()) {
			fObjectLibrary->RemoveListener(fSOMListener);
			int32 count = fObjectLibrary->CountObjects();
			for (int32 i = 0; i < count; i++)
				ObjectRemoved(fObjectLibrary->ObjectAtFast(i));
		
			fObjectLibrary->WriteUnlock();
		} else {
			printf("unable to lock object library\n");
			return;
		}
	}

	fObjectLibrary = library;

	if (!fObjectLibrary)
		return;

	if (!fObjectLibrary->WriteLock())
		return;

	fObjectLibrary->AddListener(fSOMListener);

	int32 count = fObjectLibrary->CountObjects();
	for (int32 i = 0; i < count; i++)
		ObjectAdded(fObjectLibrary->ObjectAtFast(i), i);

	fObjectLibrary->WriteUnlock();
}

// SetSelection
void
ObjectColumnListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// Sync
void
ObjectColumnListView::Sync()
{
	if (!fObjectLibrary) {
		_MakeEmpty();
		return;
	}

	if (!fObjectLibrary->WriteLock())
		return;

	// mark all items for removal
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ObjectCLVItem* item = dynamic_cast<ObjectCLVItem*>(ItemAtFast(i));
		if (!item)
			continue;
		item->removeAfterSync = true;
	}

	// find all accepted objects, try to find an already existing item,
	// unmark it for removal, if no item could be found, add it
	count = fObjectLibrary->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fObjectLibrary->ObjectAtFast(i);
		if (AcceptObject(object)) {
			ObjectCLVItem* item = ItemForObject(object);
			if (!item)
				AddObject(object);
			else
				item->removeAfterSync = false;
		} else {
			RemoveObject(object);
		}
	}

	fObjectLibrary->WriteUnlock();

	// remove any leftover items that are still marked for removal
	count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		ObjectCLVItem* item = dynamic_cast<ObjectCLVItem*>(ItemAtFast(i));
		if (!item)
			continue;
		if (item->removeAfterSync)
			delete RemoveItem(i);
	}

	Sort();
}

// UpdateItem
bool
ObjectColumnListView::UpdateItem(ObjectCLVItem* item) const
{
	if (!item)
		return false;

	ServerObject* object = item->object;
	if (!object)
		return false;

	TextColumnItem* columnItem = dynamic_cast<TextColumnItem*>(
		item->ColumnItemAt(kNameColumnIndex));

	bool removed = object->HasRemovedStatus();

	if (item->removed != removed
		|| (!columnItem || object->Name() != columnItem->Text())) {
		item->removed = removed;
		item->SetContent(kNameColumnIndex, object->Name().String(), removed);
		return true;
	}
	return false;
}

// ItemForObject
ObjectCLVItem*
ObjectColumnListView::ItemForObject(ServerObject* object) const
{
	for (int32 i = 0; ObjectCLVItem* item = dynamic_cast<ObjectCLVItem*>(ItemAt(i)); i++) {
		if (item->object == object)
			return item;
	}
	return NULL;
}

// StoreSettings
void
ObjectColumnListView::StoreSettings(BMessage* archive,
	const char* name) const
{
	BMessage settings;
	for (int32 i = 0; Column* column = ColumnAt(i); i++) {
		settings.AddInt32("ordered index", OrderedColumnIndexOf(i));
		settings.AddInt32("width", column->Width());
	}
	int32 sortIndex;
	bool reverse;
	if (PrimarySortColumn(&sortIndex, &reverse) == B_OK) {
		settings.AddInt32("primary sort index", sortIndex);
		settings.AddBool("primary sort rev", reverse);
	}

	if (SecondarySortColumn(&sortIndex, &reverse) == B_OK) {
		settings.AddInt32("secondary sort index", sortIndex);
		settings.AddBool("secondary sort rev", reverse);
	}

	archive->RemoveName(name);
	archive->AddMessage(name, &settings);
}

// RestoreSettings
void
ObjectColumnListView::RestoreSettings(BMessage* archive, const char* name)
{
	BMessage settings;
	if (archive->FindMessage(name, &settings) < B_OK)
		return;

	int32 orderedIndex;
	int32 width;
	for (int32 i = 0; Column* column = ColumnAt(i); i++) {
		if (settings.FindInt32("ordered index", i, &orderedIndex) < B_OK)
			break;
		if (settings.FindInt32("width", i, &width) < B_OK)
			break;

		MoveColumn(column, orderedIndex);
		ResizeColumn(column, width);
	}
	int32 sortIndex;
	bool reverse;
	if (settings.FindInt32("primary sort index", &sortIndex) == B_OK
		&& settings.FindBool("primary sort rev", &reverse) == B_OK)
		SetPrimarySortColumn(sortIndex, reverse);

	if (settings.FindInt32("secondary sort index", &sortIndex) == B_OK
		&& settings.FindBool("secondary sort rev", &reverse) == B_OK)
		SetSecondarySortColumn(sortIndex, reverse);

	Sort();
}

// AddObject
bool
ObjectColumnListView::AddObject(ServerObject* object, int32 index)
{
	if (object) {
		if (index < 0)
			index = CountItems();
		return AddItem(new ObjectCLVItem(object, this, fItemHeight), index);
	}
	return false;
}

// RemoveObject
bool
ObjectColumnListView::RemoveObject(ServerObject* object)
{
	ObjectCLVItem* item = ItemForObject(object);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// #pragma mark -

// ObjectAdded
void
ObjectColumnListView::ObjectAdded(ServerObject* object, int32 index)
{
	// we need to write lock the object manager, since we are going
	// to add a listener to this object
	if (!fObjectLibrary || !fObjectLibrary->WriteLock())
		return;

	// since this is an asynchronous notification, avoid the race
	// condition that this object might already be removed again
	// direct accepting of objects to descending classes
	if (fObjectLibrary->HasObject(object) && AcceptObject(object)) {
		AddObject(object);
		Sort();
	}

	fObjectLibrary->WriteUnlock();
}

// ObjectRemoved
void
ObjectColumnListView::ObjectRemoved(ServerObject* object)
{
	// we need to write lock the object manager, since we are going
	// to add a listener to this object
	if (!fObjectLibrary || !fObjectLibrary->WriteLock())
		return;

	RemoveObject(object);

	fObjectLibrary->WriteUnlock();
}

// #pragma mark -

// _MakeEmpty
void
ObjectColumnListView::_MakeEmpty()
{
	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		delete RemoveItem(i);
	}
}

// _SendSelectionChangedMessage
ObjectCLVItem*
ObjectColumnListView::_SendSelectionChangedMessage(int32 index, bool invoked)
{
	ObjectCLVItem* item = dynamic_cast<ObjectCLVItem*>(ItemAt(index));
	if (fMessage) {
		BMessage message(*fMessage);
		if (item) {
			message.AddPointer("object", (void*)item->object);
			message.AddBool("invoked", invoked);
		}
		Invoke(&message);
	}
	return item;
}


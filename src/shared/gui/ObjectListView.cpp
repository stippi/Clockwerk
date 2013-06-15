/*
 * Copyright 2006-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ObjectListView.h"

#include <stdio.h>

#include <Application.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <ScrollBar.h>
#include <String.h>
#include <Window.h>

#include "AsyncObserver.h"
#include "CommonPropertyIDs.h"
#include "ObjectItem.h"
#include "Observer.h"
#include "Selection.h"
#include "ServerObject.h"

enum {
	MSG_DRAG_OBJECT	= 'drgo',
};

// constructor
ObjectListView::ObjectListView(const char* name,
							   BMessage* message,
							   BHandler* target)
	: SimpleListView(BRect(0.0, 0.0, 20.0, 20.0), name,
					 NULL, B_SINGLE_SELECTION_LIST),
	  fMessage(message),
	  fObjectLibrary(NULL),
	  fSelection(NULL),
	  fSOMListener(new AsyncSOMListener(this))
{
	SetDragCommand(MSG_DRAG_OBJECT);
	SetTarget(target);
}

// destructor
ObjectListView::~ObjectListView()
{
	_MakeEmpty();
	delete fMessage;

	if (fObjectLibrary)
		fObjectLibrary->RemoveListener(fSOMListener);

	delete fSOMListener;
}

// SelectionChanged
void
ObjectListView::SelectionChanged()
{
	// NOTE: this is a single selection list!

	ObjectItem* item = dynamic_cast<ObjectItem*>(
		ItemAt(CurrentSelection(0)));
	if (fMessage) {
		BMessage message(*fMessage);
		if (item)
			message.AddPointer("object", (void*)item->object);
		Invoke(&message);
	}

	// modify global Selection
	if (!fSelection)
		return;

//	if (item)
//		fSelection->Select(item->object);
//	else
//		fSelection->DeselectAll();
}

// MessageReceived
void
ObjectListView::MessageReceived(BMessage* message)
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
					ObjectItem* item = _ItemForObject(object);
					if (item && UpdateItem(item))
						InvalidateItem(IndexOf(item));
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
			SimpleListView::MessageReceived(message);
			break;
	}
}

// MakeDragMessage
void
ObjectListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("library", fObjectLibrary);
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ObjectItem* item = dynamic_cast<ObjectItem*>(ItemAt(CurrentSelection(i)));
		if (item) {
			message->AddPointer("object", (void*)item->object);
		} else
			break;
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
}

// AcceptDragMessage
bool
ObjectListView::AcceptDragMessage(const BMessage* message) const
{
	// don't allow any dropping for now
	return false;
}

// #pragma mark -

// CopyItems
void
ObjectListView::CopyItems(BList& items, int32 toIndex)
{
	MoveItems(items, toIndex);
	// copy operation not allowed -> ?!?
}

// RemoveItemList
void
ObjectListView::RemoveItemList(BList& indices)
{
	// not allowed
}

// CloneItem
BListItem*
ObjectListView::CloneItem(int32 index) const
{
	if (ObjectItem* item = dynamic_cast<ObjectItem*>(ItemAt(index))) {
		return new ObjectItem(item->object,
							  const_cast<ObjectListView*>(this));
	}
	return NULL;
}

// #pragma mark -

// SetObjectLibrary
void
ObjectListView::SetObjectLibrary(ServerObjectManager* library)
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
ObjectListView::SetSelection(Selection* selection)
{
	fSelection = selection;
}

// Sync
void
ObjectListView::Sync()
{
// Begin hack to make list modification faster
	if (Window())
		Window()->UpdateIfNeeded();
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar)
		scrollBar->SetTarget((BView*)NULL);
// End hack to make list modification faster

	uint32 removeAfterSyncFlag = 1 << 31;

	// mark all items for removal
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ObjectItem* item = dynamic_cast<ObjectItem*>(ItemAt(i));
		if (!item)
			continue;
		item->flags |= removeAfterSyncFlag;
	}

	if (fObjectLibrary && fObjectLibrary->WriteLock()) {
		// find all accepted objects, try to find an already existing item,
		// unmark it for removal, if no item could be found, add it
		count = fObjectLibrary->CountObjects();
		for (int32 i = 0; i < count; i++) {
			ServerObject* object = fObjectLibrary->ObjectAtFast(i);
			if (AcceptObject(object)) {
				ObjectItem* item = _ItemForObject(object);
				if (!item)
					_AddObject(object, _InsertIndexFor(object));
				else
					item->flags &= ~removeAfterSyncFlag;
			} else {
				_RemoveObject(object);
			}
		}
	
		fObjectLibrary->WriteUnlock();
	}

	// remove any leftover items that are still marked for removal
	count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		ObjectItem* item = dynamic_cast<ObjectItem*>(ItemAt(i));
		if (!item)
			continue;
		if (item->flags & removeAfterSyncFlag)
			delete RemoveItem(i);
	}

// Begin hack to make list modification faster
	if (scrollBar)
		scrollBar->SetTarget(this);
	BListView::FrameResized(Bounds().Width(), Bounds().Height());
// End hack to make list modification faster
}

// UpdateItem
bool
ObjectListView::UpdateItem(ObjectItem* item) const
{
	if (!item)
		return false;

	ServerObject* object = item->object;
	if (!object)
		return false;

	uint32 flags = item->flags;
	if (object->HasRemovedStatus())
		flags |= 0x01;
	else
		flags &= ~0x01;

	if (item->flags != flags || object->Name() != item->Text()) {
		item->flags = flags;
		item->SetText(object->Name().String());
		return true;
	}
	return false;
}

// SetupDrawFont
void
ObjectListView::SetupDrawFont(BView* view, const ObjectItem* item, BFont* font)
{
	view->GetFont(font);

	// configure font and color according to "removed" status
	if (item->flags & 0x01) {
		font->SetShear(105.0);
		font->SetSpacing(B_STRING_SPACING);
		view->SetFont(font, B_FONT_SHEAR | B_FONT_SPACING);
		view->SetHighColor(128, 128, 128, 255);
	} else {
		view->SetHighColor(0, 0, 0, 255);
	}
}

// #pragma mark -

// ObjectAdded
void
ObjectListView::ObjectAdded(ServerObject* object, int32 index)
{
	// we need to write lock the object manager, since we are going
	// to add a listener to this object
	if (!fObjectLibrary || !fObjectLibrary->WriteLock())
		return;

	// since this is an asynchronous notification, avoid the race
	// condition that this object might already be removed again
	// direct accepting of objects to descending classes
	if (fObjectLibrary->HasObject(object) && AcceptObject(object))
		_AddObject(object, _InsertIndexFor(object));

	fObjectLibrary->WriteUnlock();
}

// ObjectRemoved
void
ObjectListView::ObjectRemoved(ServerObject* object)
{
	// we need to write lock the object manager, since we are going
	// to add a listener to this object
	if (!fObjectLibrary || !fObjectLibrary->WriteLock())
		return;

	_RemoveObject(object);

	fObjectLibrary->WriteUnlock();
}

// #pragma mark -

// _AddObject
bool
ObjectListView::_AddObject(ServerObject* object, int32 index)
{
	if (object) {
		if (index < 0)
			index = CountItems();
		return AddItem(new ObjectItem(object, this), index);
	}
	return false;
}

// _RemoveObject
bool
ObjectListView::_RemoveObject(ServerObject* object)
{
	ObjectItem* item = _ItemForObject(object);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// _ItemForObject
ObjectItem*
ObjectListView::_ItemForObject(ServerObject* object) const
{
	for (int32 i = 0; ObjectItem* item = dynamic_cast<ObjectItem*>(ItemAt(i)); i++) {
		if (item->object == object)
			return item;
	}
	return NULL;
}

// _MakeEmpty
void
ObjectListView::_MakeEmpty()
{
	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		delete RemoveItem(i);
	}
}

// _InsertIndexFor
int32
ObjectListView::_InsertIndexFor(ServerObject* object) const
{
	// binary search
	int32 lower = 0;
	int32 upper = CountItems();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		ObjectItem* item = dynamic_cast<ObjectItem*>(ItemAt(mid));
		if (strcasecmp(item->Text(), object->Name().String()) > 0)
			upper = mid;
		else
			lower = mid + 1;
	}
	return lower;
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ObjectCLVItem.h"

#include <stdio.h>

#include <String.h>

#include "AsyncObserver.h"
#include "ObjectColumnListView.h"
#include "ServerObject.h"

// constructor
ObjectCLVItem::ObjectCLVItem(ServerObject* o, ObjectColumnListView* listView,
		float height)
	: EasyColumnListItem(height)
	, object(NULL)
	, removed(false)

	, removeAfterSync(false)

	, fListView(listView)
	, fObserver(new AsyncObserver(listView))
{
	SetObject(o);
}

// destructor
ObjectCLVItem::~ObjectCLVItem()
{
	SetObject(NULL);
	fObserver->Release();
}

// SetObject
void
ObjectCLVItem::SetObject(ServerObject* o)
{
	if (o == object)
		return;

	if (object) {
		object->RemoveObserver(fObserver);
		object->Release();
	}

	object = o;

	if (object) {
		object->Acquire();
		object->AddObserver(fObserver);
		fListView->UpdateItem(this);
	}
}



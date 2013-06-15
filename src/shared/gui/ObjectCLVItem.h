/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef OBJECT_CLV_ITEM_H
#define OBJECT_CLV_ITEM_H

#include "EasyColumnListItem.h"

class AsyncObserver;
class ObjectColumnListView;
class Observable;
class ServerObject;

class ObjectCLVItem : public EasyColumnListItem {
 public:
								ObjectCLVItem(ServerObject* object,
									ObjectColumnListView* listView,
									float height);

	virtual						~ObjectCLVItem();

	// ObjectCLVItem
			void				SetObject(ServerObject* object);

			ServerObject* 		object;
			bool				removed;

			bool				removeAfterSync;

 private:
			ObjectColumnListView* fListView;
			AsyncObserver*		fObserver;
};

#endif // OBJECT_CLV_ITEM_H

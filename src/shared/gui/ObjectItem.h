/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef OBJECT_ITEM_H
#define OBJECT_ITEM_H

#include "ObjectListView.h"

class AsyncObserver;
class ObjectListView;
class Observable;
class ServerObject;

class ObjectItem : public SimpleItem {
 public:
								ObjectItem(ServerObject* object,
									ObjectListView* listView);

	virtual						~ObjectItem();

	// SimpleItem interface
	virtual	void				Draw(BView* owner, BRect itemFrame,
									uint32 flags);

	// ObjectItem
			void				SetObject(ServerObject* object);
			void				Invalidate();

			ServerObject* 		object;
			uint32				flags;

 private:
			ObjectListView*		fListView;
			AsyncObserver*		fObserver;

 public:
	static	float				sTextOffset;
	static	float				sBorderSpacing;
};

#endif // OBJECT_ITEM_H

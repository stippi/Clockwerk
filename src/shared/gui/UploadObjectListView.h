/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef UPDATE_OBJECT_LIST_VIEW_H
#define UPDATE_OBJECT_LIST_VIEW_H

#include "ObjectColumnListView.h"

class UploadObjectListView : public ObjectColumnListView {
 public:
								UploadObjectListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~UploadObjectListView();

	// ObjectListView interface
	virtual	bool				AcceptObject(ServerObject* object);

 protected:
	virtual	bool				UpdateItem(ObjectCLVItem* item) const;

 public:
	// UploadObjectListView

 private:
			bool				_SetColumnContent(ObjectCLVItem* item,
									int32 columnIndex, BString content) const;

			int32				fIDColumnIndex;
			int32				fTypeColumnIndex;
			int32				fVersionColumnIndex;
};

#endif // UPDATE_OBJECT_LIST_VIEW_H

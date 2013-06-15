/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "UploadObjectListView.h"

#include <stdio.h>

#include "Column.h"
#include "ColumnItem.h"
#include "ObjectCLVItem.h"
#include "ServerObject.h"

// constructor
UploadObjectListView::UploadObjectListView(const char* name,
		BMessage* message, BHandler* target)
	: ObjectColumnListView(name, message, target)
{
	int32 columnIndex = CountColumns();

	fIDColumnIndex = columnIndex++;
	AddColumn(new Column("ID", "id", 120.0,
		COLUMN_VISIBLE | COLUMN_SORT_KEYABLE), fIDColumnIndex);
	
	fTypeColumnIndex = columnIndex++;
	AddColumn(new Column("Type", "type", 90.0,
		COLUMN_VISIBLE | COLUMN_SORT_KEYABLE), fTypeColumnIndex);

	fVersionColumnIndex = columnIndex++;
	AddColumn(new Column("Version", "version", 40.0,
		COLUMN_VISIBLE | COLUMN_SORT_KEYABLE), fVersionColumnIndex);
}

// destructor
UploadObjectListView::~UploadObjectListView()
{
}

// AcceptObject
bool
UploadObjectListView::AcceptObject(ServerObject* object)
{
	return true;
}

// UpdateItem
bool
UploadObjectListView::UpdateItem(ObjectCLVItem* item) const
{
	if (!item)
		return false;

	bool invalidate = ObjectColumnListView::UpdateItem(item);
	if (_SetColumnContent(item, fIDColumnIndex, item->object->ID()))
		invalidate = true;
	if (_SetColumnContent(item, fTypeColumnIndex, item->object->Type()))
		invalidate = true;
	BString version;
	version << item->object->Version();
	if (_SetColumnContent(item, fVersionColumnIndex, version))
		invalidate = true;

	return invalidate;
}

// #pragma mark -

// _SetColumnContent
bool
UploadObjectListView::_SetColumnContent(ObjectCLVItem* item,
	int32 columnIndex, BString content) const
{
	TextColumnItem* columnItem = dynamic_cast<TextColumnItem*>(
		item->ColumnItemAt(columnIndex));

	if (!columnItem || content != columnItem->Text()) {
		item->SetContent(columnIndex, content.String(), false);
		return true;
	}
	return false;
}



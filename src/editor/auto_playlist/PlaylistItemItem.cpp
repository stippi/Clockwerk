/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistItemItem.h"

#include <stdio.h>

#include <String.h>

#include "PlaylistItem.h"

// constructor
PlaylistItemItem::PlaylistItemItem(PlaylistItem* item, SimpleListView* listView)
	: SimpleItem(""),
	  item(NULL),
	  fListView(listView)
{
	SetItem(item);
}

// destructor
PlaylistItemItem::~PlaylistItemItem()
{
	SetItem(NULL);
}

// ObjectChanged
void
PlaylistItemItem::ObjectChanged(const Observable* item)
{
	UpdateText();
}

// SetItem
void
PlaylistItemItem::SetItem(PlaylistItem* item)
{
	if (item == this->item)
		return;

	if (this->item) {
		this->item->RemoveObserver(this);
	}

	this->item = item;

	if (this->item) {
		this->item->AddObserver(this);
		UpdateText();
	}
}

// UpdateText
void
PlaylistItemItem::UpdateText()
{
	SetText(item->Name().String());
	// :-/
	if (fListView->LockLooper()) {
		fListView->InvalidateItem(
			fListView->IndexOf(this));
		fListView->UnlockLooper();
	}
}

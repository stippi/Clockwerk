/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ObjectItem.h"

#include <stdio.h>

#include <String.h>

#include "AsyncObserver.h"
#include "CommonPropertyIDs.h"
#include "ServerObject.h"

float
ObjectItem::sTextOffset = -1;

float
ObjectItem::sBorderSpacing = -1;

// constructor
ObjectItem::ObjectItem(ServerObject* o, ObjectListView* listView)
	: SimpleItem("")

	, object(NULL)
	, flags(0)

	, fListView(listView)
	, fObserver(new AsyncObserver(listView))
{
	SetObject(o);
}

// destructor
ObjectItem::~ObjectItem()
{
	SetObject(NULL);
	fObserver->Release();
}

// SetObject
void
ObjectItem::SetObject(ServerObject* o)
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
		if (fListView->UpdateItem(this))
			Invalidate();
	}
}

// Draw
void
ObjectItem::Draw(BView* owner, BRect itemFrame, uint32 flags)
{
	SimpleItem::DrawBackground(owner, itemFrame, flags);

	if (sTextOffset == -1)
		sTextOffset = be_plain_font->Size() / 2.0;

	if (sBorderSpacing == -1)
		sBorderSpacing = be_plain_font->Size() / 3.0;


	owner->PushState();

	BFont font;
	fListView->SetupDrawFont(owner, this, &font);

	// truncate text to fit
	const char* text = Text();
	BString truncatedString(text);
	owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
		itemFrame.Width() - sBorderSpacing - sTextOffset - sBorderSpacing);

	// figure out text position
	font_height fh;
	font.GetHeight(&fh);

	float height = itemFrame.Height();
	float textHeight = fh.ascent + fh.descent;
	BPoint pos;
	pos.x = itemFrame.left + sBorderSpacing + sTextOffset;
	pos.y = itemFrame.top + ceilf(height / 2.0 - textHeight / 2.0 + fh.ascent);

	// draw text
	owner->DrawString(truncatedString.String(), pos);

	owner->PopState();
}

// Invalidate
void
ObjectItem::Invalidate()
{
	fListView->InvalidateItem(fListView->IndexOf(this));
}


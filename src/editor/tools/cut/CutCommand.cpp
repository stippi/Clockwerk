/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CutCommand.h"

#include <new>

#include "Playlist.h"
#include "PlaylistItem.h"

using std::nothrow;

// constructor
CutCommand::CutCommand(PlaylistItem* item, int64 cutFrame)
	: Command(),
	  fOriginalItem(item),
	  fInsertedItem(NULL)
{
	if (fOriginalItem
		&& fOriginalItem->Parent()
		&& cutFrame > fOriginalItem->StartFrame()
		&& cutFrame < fOriginalItem->EndFrame()) {

		// clone the orginal item
		fInsertedItem = fOriginalItem->Clone(true);
		if (fInsertedItem) {
			// move the inserted items clip offset (startframe)
			// to the cut pos
			fInsertedItem->SetClipOffset((cutFrame - fOriginalItem->StartFrame())
										 + fInsertedItem->ClipOffset());
		}
	}
}

// destructor
CutCommand::~CutCommand()
{
}

// InitCheck
status_t
CutCommand::InitCheck()
{
	return fOriginalItem && fOriginalItem->Parent()
		   && fInsertedItem ? B_OK : B_NO_INIT;
}

// Perform
status_t
CutCommand::Perform()
{
	status_t ret = InitCheck();
	if (ret == B_OK) {
		fOriginalItem->SetDuration(fOriginalItem->Duration()
								   - fInsertedItem->Duration());
		fOriginalItem->Parent()->AddItem(fInsertedItem);
	}
	return ret;
}

// Undo
status_t
CutCommand::Undo()
{
	status_t ret = InitCheck();
	if (ret == B_OK) {
		fOriginalItem->SetDuration(fOriginalItem->Duration()
								   + fInsertedItem->Duration());
		fOriginalItem->Parent()->RemoveItem(fInsertedItem);
	}
	return ret;
}

// GetName
void
CutCommand::GetName(BString& name)
{
	name << "Cut Clip";
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ReplaceClipCommand.h"

#include <new>

#include "Clip.h"
#include "ClipPlaylistItem.h"

using std::nothrow;

// constructor
ReplaceClipCommand::ReplaceClipCommand(ClipPlaylistItem* item,
									   Clip* newClip)
	: Command(),
	  fItem(item),
	  fOldClip(item ? item->Clip() : NULL),
	  fOldDuration(item ? item->Duration() : 0),
	  fNewClip(newClip)
{
	if (fOldClip)
		fOldClip->Acquire();
	if (fNewClip)
		fNewClip->Acquire();
}

// destructor
ReplaceClipCommand::~ReplaceClipCommand()
{
	if (fOldClip)
		fOldClip->Release();
	if (fNewClip)
		fNewClip->Release();
}

// InitCheck
status_t
ReplaceClipCommand::InitCheck()
{
	return fItem && fNewClip ? B_OK : B_NO_INIT;
}

// Perform
status_t
ReplaceClipCommand::Perform()
{
	fItem->SetClip(fNewClip);

	return B_OK;
}

// Undo
status_t
ReplaceClipCommand::Undo()
{
	AutoNotificationSuspender _(fItem);

	fItem->SetClip(fOldClip);
	fItem->SetDuration(fOldDuration);

	return B_OK;
}

// GetName
void
ReplaceClipCommand::GetName(BString& name)
{
	name << "Replace Clip";
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ReplaceClipDropState.h"

#include <new>
#include <stdio.h>

#include <Alert.h>

#include "Clip.h"
#include "ClipPlaylistItem.h"
#include "ReplaceClipCommand.h"
#include "Playlist.h"
#include "ServerObjectManager.h"
#include "TimelineView.h"
#include "TimelineMessages.h"

using std::nothrow;

// constructor
ReplaceClipDropState::ReplaceClipDropState(TimelineView* view)
	: DropAnticipationState(view),
	  fView(view),

	  fTargetItem(NULL)
{
}

// destructor
ReplaceClipDropState::~ReplaceClipDropState()
{
}

// #pragma mark -

// Draw
void
ReplaceClipDropState::Draw(BView* into, BRect updateRect)
{
	if (!fDropAnticipationRect.IsValid())
		return;

	BRect r = fDropAnticipationRect;
	r.InsetBy(1, 1);
	if (r.IsValid()) {
		into->SetHighColor(0, 0, 0, 30);
		into->SetDrawingMode(B_OP_ALPHA);
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		into->FillRect(fDropAnticipationRect);
	}

	into->SetHighColor(255, 0, 0, 255);
	into->SetDrawingMode(B_OP_COPY);
	into->StrokeRect(fDropAnticipationRect);
}

// #pragma mark -

// WouldAcceptDragMessage
bool
ReplaceClipDropState::WouldAcceptDragMessage(const BMessage* dragMessage)
{
	if (dragMessage->what != MSG_DRAG_CLIP)
		return false;

	// inspect the message to retrieve the number of clips
	// we only support dropping one clip

	ServerObjectManager* library;
	if (dragMessage->FindPointer("library", (void**)&library) != B_OK)
		return false;

	if (!library || !library->ReadLock())
		return false;

	Clip* clip;
	int32 foundClips = 0;
	for (int32 i = 0; dragMessage->FindPointer("clip", i,
											   (void**)&clip) == B_OK; i++) {
		if (!library->HasObject(clip))
			continue;

		foundClips++;
		if (foundClips > 1)
			break;
	}

	library->ReadUnlock();

	return foundClips == 1;
}

// HandleDropMessage
Command*
ReplaceClipDropState::HandleDropMessage(BMessage* dropMessage)
{
	if (fTargetItem == NULL)
		return NULL;

	if (dropMessage->what == MSG_DRAG_CLIP) {

		// inspect the message to retrieve the clips
		ServerObjectManager* library;
		if (dropMessage->FindPointer("library", (void**)&library) != B_OK)
			return NULL;
	
		if (!library || !library->ReadLock())
			return NULL;

		Clip* replacementClip = NULL;

		Clip* clip;
		for (int32 i = 0; dropMessage->FindPointer("clip", i,
												   (void**)&clip) == B_OK; i++) {
			if (!library->HasObject(clip)) {
				// the message has arrived asynchronously,
				// so the clip pointer might be stale
				continue;
			}

			Playlist* playlist = dynamic_cast<Playlist*>(clip);
			if (playlist && playlist == fView->Playlist()) {
				BAlert* alert = new BAlert("error", "You probably do not "
					"want to insert a playlist into itself.", "Insert",
					"Cancel", NULL, B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
				int32 choice = alert->Go();
				if (choice != 0)
					continue;
			}

			replacementClip = clip;
			break;
		}

		library->ReadUnlock();

		return new (nothrow) ReplaceClipCommand(fTargetItem, replacementClip);
	}
	return NULL;
}

// UpdateDropIndication
void
ReplaceClipDropState::UpdateDropIndication(const BMessage* dragMessage,
										   BPoint where, uint32 modifiers)
{
	uint32 dropTrack = 0;

	// figure out track
	float trackHeight = fView->TrackHeight();
	float y = trackHeight;
	while (where.y > y) {
		y += trackHeight;
		dropTrack++;
	}

	int64 dropFrame = fView->FrameForPos(where.x);

	// find out item at given frame
	fTargetItem = NULL;
	Playlist* playlist = fView->Playlist();
	int32 count = playlist->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = playlist->ItemAtFast(i);
		ClipPlaylistItem* clipItem = dynamic_cast<ClipPlaylistItem*>(item);
		if (!clipItem)
			continue;
		if (item->Track() == dropTrack
			&& item->StartFrame() <= dropFrame
			&& item->EndFrame() >= dropFrame) {
			fTargetItem = clipItem;
			break;
		}
	}

	if (fTargetItem) {
		float startX = fView->PosForFrame(fTargetItem->StartFrame());
		float endX = fView->PosForFrame(fTargetItem->EndFrame()) + 1;
		SetDropAnticipationRect(BRect(startX, y - trackHeight, endX, y));
	} else {
		SetDropAnticipationRect(BRect(0, 0, -1, -1));
	}
}

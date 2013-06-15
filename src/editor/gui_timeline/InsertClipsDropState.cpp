/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InsertClipsDropState.h"

#include <new>
#include <stdio.h>

#include <Alert.h>

#include "Clip.h"
#include "ClipPlaylistItem.h"
#include "InsertCommand.h"
#include "ItemForClipFactory.h"
#include "Playlist.h"
#include "ServerObjectManager.h"
#include "TimelineView.h"
#include "TimelineMessages.h"

using std::nothrow;

// constructor
InsertClipsDropState::InsertClipsDropState(TimelineView* view)
	: DropAnticipationState(view),
	  fView(view),

	  fSnapFrames(),

	  fDropFrame(0),
	  fDropTrack(0),
	  fDraggedClipDuration(0)
{
}

// destructor
InsertClipsDropState::~InsertClipsDropState()
{
}

// #pragma mark -

//// Init
//void
//InsertClipsDropState::Init()
//{
//  fDropFrame = 0;
//  fDropTrack = 0;
//  fDraggedClipDuration = 0;
//}
//
//// Cleanup
//void
//InsertClipsDropState::Cleanup()
//{
//  fDropFrame = 0;
//  fDropTrack = 0;
//  fDraggedClipDuration = 0;
//}

// #pragma mark -

// Draw
void
InsertClipsDropState::Draw(BView* into, BRect updateRect)
{
	if (!fDropAnticipationRect.IsValid())
		return;

	BRect r = fDropAnticipationRect;
	r.right--;
	if (r.IsValid()) {
		into->SetHighColor(0, 0, 0, 30);
		into->SetDrawingMode(B_OP_ALPHA);
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
		into->FillRect(fDropAnticipationRect);
	}

	into->SetHighColor(255, 0, 0, 255);
	into->SetDrawingMode(B_OP_COPY);
	into->StrokeLine(fDropAnticipationRect.LeftTop(),
					 fDropAnticipationRect.LeftBottom());
}

// #pragma mark -

// WouldAcceptDragMessage
bool
InsertClipsDropState::WouldAcceptDragMessage(const BMessage* dragMessage)
{
	if (dragMessage->what != MSG_DRAG_CLIP)
		return false;

	// inspect the message to retrieve the clip duration
	ServerObjectManager* library;
	if (dragMessage->FindPointer("library", (void**)&library) != B_OK)
		return false;

	if (!library || !library->ReadLock())
		return false;

	fDraggedClipDuration = 0;

	Clip* clip;
	for (int32 i = 0; dragMessage->FindPointer("clip", i,
											   (void**)&clip) == B_OK; i++) {
		if (!library->HasObject(clip))
			continue;
		fDraggedClipDuration += ClipPlaylistItem::DefaultDuration(clip->Duration());
	}

	library->ReadUnlock();

	fSnapFrames.CollectSnapFrames(fView->Playlist(), fDraggedClipDuration);
	fSnapFrames.AddSnapFrame(0, fDraggedClipDuration);
	if (fView->IsPaused())
		fSnapFrames.AddSnapFrame(fView->CurrentFrame(), fDraggedClipDuration);

	return true;
}

// HandleDropMessage
Command*
InsertClipsDropState::HandleDropMessage(BMessage* dropMessage)
{
	if (dropMessage->what == MSG_DRAG_CLIP) {

		// inspect the message to retrieve the clips
		ServerObjectManager* library;
		if (dropMessage->FindPointer("library", (void**)&library) != B_OK)
			return NULL;
	
		if (!library || !library->ReadLock())
			return NULL;

		// temporary list to hold the created items
		BList items;

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

			PlaylistItem* item = gItemForClipFactory->PlaylistItemForClip(clip);

			if (!items.AddItem(item)) {
				fprintf(stderr, "InsertClipsDropState::HandleDropMessage() "
								"no memory to insert item in list\n");
				delete item;
				break;
			}
		}

		library->ReadUnlock();

		return new (nothrow) InsertCommand(fView->Playlist(),
										   fView->Selection(),
										   (PlaylistItem**)items.Items(),
										   items.CountItems(),
										   fDropFrame, fDropTrack);
	}
	return NULL;
}

// UpdateDropIndication
void
InsertClipsDropState::UpdateDropIndication(const BMessage* dragMessage,
											BPoint where, uint32 modifiers)
{
	fDropTrack = 0;

	// figure out track
	float trackHeight = fView->TrackHeight();
	float y = trackHeight;
	while (where.y > y) {
		y += trackHeight;
		fDropTrack++;
	}

	fDropFrame = fView->FrameForPos(where.x);
	// snapping
	fDropFrame = fSnapFrames.ClosestFrameFor(fDropFrame, fDropTrack,
		fView->ZoomLevel());

	int64 endFrame = fDropFrame + fDraggedClipDuration;

	float startX = fView->PosForFrame(fDropFrame);
	float endX = fView->PosForFrame(endFrame);

	SetDropAnticipationRect(BRect(startX, y - trackHeight, endX, y - 1));
}

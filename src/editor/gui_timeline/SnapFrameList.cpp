/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SnapFrameList.h"

#include <new>

#include <stdlib.h>

#include "Playlist.h"
#include "PlaylistItem.h"

using std::nothrow;

struct snap_frame {
	int64	startSnapFrame;
	int64	endSnapFrame;
	uint32	track;
};

// constructor
SnapFrameList::SnapFrameList()
	: fSnapFrames(24)
{
}

// destructor
SnapFrameList::~SnapFrameList()
{
	_MakeEmpty();
}

// CollectSnapFrames
void
SnapFrameList::CollectSnapFrames(const Playlist* list,
								 uint64 snapToEndOffset)
{
	if (!list)
		return;

	_MakeEmpty();

	int32 count = list->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = list->ItemAtFast(i);
		snap_frame* startSnap = new (nothrow) snap_frame;
		if (!startSnap || !fSnapFrames.AddItem((void*)startSnap)) {
			delete startSnap;
			break;
		}
		snap_frame* endSnap = new (nothrow) snap_frame;
		if (!endSnap || !fSnapFrames.AddItem((void*)endSnap)) {
			delete endSnap;
			break;
		}
		// if snapToEndOffset is specified,
		// add a snap frame snapToEndOffset frames
		// before the items end frame, so that we
		// can snap there too (which will result in
		// something being aligned to the end instead)
		startSnap->startSnapFrame = item->StartFrame();
		startSnap->endSnapFrame = startSnap->startSnapFrame - snapToEndOffset;
		endSnap->startSnapFrame = item->EndFrame() + 1;
		endSnap->endSnapFrame = endSnap->startSnapFrame - snapToEndOffset;
			// don't snap on the >end< frame, but on the next
		startSnap->track = endSnap->track = item->Track();
	}
}

// AddSnapFrame
void
SnapFrameList::AddSnapFrame(int64 frame, uint64 snapToEndOffset)
{
	snap_frame* startSnap = new (nothrow) snap_frame;
	if (!startSnap || !fSnapFrames.AddItem((void*)startSnap)) {
		delete startSnap;
		return;
	}
	// if snapToEndOffset is specified,
	// add a snap frame snapToEndOffset frames
	// before the items end frame, so that we
	// can snap there too (which will result in
	// something being aligned to the end instead)
	startSnap->startSnapFrame = frame;
	startSnap->endSnapFrame = startSnap->startSnapFrame - snapToEndOffset;
	startSnap->track = 0;
}

// ClosestFrameFor
int64
SnapFrameList::ClosestFrameFor(int64 frame, uint32 track, double zoomLevel) const
{
	int64 closest = frame;
	int32 snapDist = (int32)ceil(8 * zoomLevel);

	int32 count = fSnapFrames.CountItems();
	for (int32 i = 0; i < count; i++) {
		snap_frame* snapFrame = (snap_frame*)fSnapFrames.ItemAtFast(i);
		if (abs(snapFrame->startSnapFrame - frame) < snapDist) {
			closest = snapFrame->startSnapFrame;
		} else if (abs(snapFrame->endSnapFrame - frame) < snapDist) {
			closest = snapFrame->endSnapFrame;
		}
	}
	return closest;
}

// ClosestSnapFrameFor
int64
SnapFrameList::ClosestSnapFrameFor(int64 frame) const
{
	int64 closest = frame;
	int64 minDist = LONG_MAX;

	int32 count = fSnapFrames.CountItems();
	for (int32 i = 0; i < count; i++) {
		snap_frame* snapFrame = (snap_frame*)fSnapFrames.ItemAtFast(i);
		int64 distStart = abs(snapFrame->startSnapFrame - frame);
		int64 distEnd = abs(snapFrame->endSnapFrame - frame);
		if (distStart < minDist) {
			closest = snapFrame->startSnapFrame;
			minDist = distStart;
		} else if (distEnd < minDist) {
			closest = snapFrame->endSnapFrame;
			minDist = distEnd;
		}
	}
	return closest;
}

// ClosestSnapFrameBackwardsFor
int64
SnapFrameList::ClosestSnapFrameBackwardsFor(int64 frame) const
{
	int64 closest = frame;
	int64 minDist = LONG_MAX;

	int32 count = fSnapFrames.CountItems();
	for (int32 i = 0; i < count; i++) {
		snap_frame* snapFrame = (snap_frame*)fSnapFrames.ItemAtFast(i);
		int64 distStart = frame - snapFrame->startSnapFrame;
		int64 distEnd = frame - snapFrame->endSnapFrame;
		if (distStart > 0 && distStart < minDist) {
			closest = snapFrame->startSnapFrame;
			minDist = distStart;
		} else if (distEnd > 0 && distEnd < minDist) {
			closest = snapFrame->endSnapFrame;
			minDist = distEnd;
		}
	}
	return closest;
}

// ClosestSnapFrameForwardFor
int64
SnapFrameList::ClosestSnapFrameForwardFor(int64 frame) const
{
	int64 closest = frame;
	int64 minDist = LONG_MAX;

	int32 count = fSnapFrames.CountItems();
	for (int32 i = 0; i < count; i++) {
		snap_frame* snapFrame = (snap_frame*)fSnapFrames.ItemAtFast(i);
		int64 distStart = snapFrame->startSnapFrame - frame;
		int64 distEnd = snapFrame->endSnapFrame - frame;
		if (distStart > 0 && distStart < minDist) {
			closest = snapFrame->startSnapFrame;
			minDist = distStart;
		} else if (distEnd > 0 && distEnd < minDist) {
			closest = snapFrame->endSnapFrame;
			minDist = distEnd;
		}
	}
	return closest;
}

// _MakeEmpty
void
SnapFrameList::_MakeEmpty()
{
	int32 count = fSnapFrames.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (snap_frame*)fSnapFrames.ItemAtFast(i);
	fSnapFrames.MakeEmpty();
}


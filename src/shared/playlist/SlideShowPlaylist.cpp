/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SlideShowPlaylist.h"

#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "PlaylistItem.h"
#include "PropertyAnimator.h"


// constructor 
SlideShowPlaylist::SlideShowPlaylist()
	: Playlist("SlideShowPlaylist")
{
}

// constructor 
SlideShowPlaylist::SlideShowPlaylist(const SlideShowPlaylist& other)
	: Playlist(other, true)
{
}

// destructor
SlideShowPlaylist::~SlideShowPlaylist()
{
}

// ValidateItemLayout
void
SlideShowPlaylist::ValidateItemLayout()
{
	int64 duration = Value(PROPERTY_DURATION, (int64)0);
	if (duration == 0)
		return;

	int64 transitionDuration
		= Value(PROPERTY_TRANSITION_DURATION, (int64)0);
	// TODO: transition mode...

	int32 count = CountItems();

	BList managedItems;

	int64 minDuration = 0;
	int64 minItemDuration = transitionDuration * 3;
	int64 fixedItemsDuration = 0;
	int64 maxDuration = 0;
	int32 variableItemCount = 0;
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		if (item->Track() > 1)
			// not a "managed" item
			continue;

		managedItems.AddItem(item);

		if (item->HasMaxDuration()) {
			int64 maxItemDuration = item->MaxDuration();
			minDuration += maxItemDuration;
			fixedItemsDuration += maxItemDuration;
			if (minItemDuration > maxItemDuration)
				minItemDuration = maxItemDuration;
		} else {
			minDuration += 3 * transitionDuration;
			variableItemCount++;
		}

		maxDuration += item->MaxDuration();
	}

	count = managedItems.CountItems();
	if (count == 0)
		return;

	if (duration < minDuration)
		duration = minDuration;
	if (duration > maxDuration)
		duration = maxDuration;

	// limit transition duration to 1/3 of the minimum item duration
	int64 maxTransitionDuration = minItemDuration / 3;

	if (transitionDuration > maxTransitionDuration)
		transitionDuration = maxTransitionDuration;

	int64 variableItemsDuration = duration - fixedItemsDuration
			+ transitionDuration * (count - variableItemCount);

	int64 startFrame = 0;
	int64 lastVariableStartFrame = 0;
	int32 variableItemIndex = 0;
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = (PlaylistItem*)managedItems.ItemAtFast(i);
		// overlapping items
		item->SetClipOffset(0);
		item->SetTrack(i & 1);

		int64 nextStartFrame;
		if (item->HasMaxDuration()) {
			nextStartFrame = startFrame + item->MaxDuration()
								- transitionDuration;
		} else {
			variableItemIndex++;
			int64 nextVariableStartFrame = (variableItemsDuration - transitionDuration)
								* variableItemIndex / variableItemCount;
			nextStartFrame = startFrame + nextVariableStartFrame - lastVariableStartFrame;
			lastVariableStartFrame = nextVariableStartFrame;
		}
		item->SetStartFrame(startFrame);
		item->SetDuration(nextStartFrame - startFrame + transitionDuration);
		startFrame = nextStartFrame;

		// transition
		PropertyAnimator* animator = item->AlphaAnimator();
		if (!animator)
			continue;

		AutoNotificationSuspender _(animator);

		// remove all keyframes to get a clean start
		animator->MakeEmpty();
		KeyFrame* first = animator->InsertKeyFrameAt(0LL);
		KeyFrame* last = animator->InsertKeyFrameAt(item->Duration() - 1);

		if (!first || !last)
			continue;

		first->SetScale(1.0);
		last->SetScale(1.0);

		// transition in top items
		if (transitionDuration > 0 && !(i & 1)) {
			// item on first track, animated opacity property
			if (i > 0) {
				// fade in
				KeyFrame* key = animator->InsertKeyFrameAt(transitionDuration);
				key->SetScale(1.0);
				first->SetScale(0.0);
			}
			
			if (i < count - 1) {
				// fade out
				KeyFrame* key = animator->InsertKeyFrameAt(
									item->Duration() - 1 - transitionDuration);
				key->SetScale(1.0);
				last->SetScale(0.0);
			}
		}
	}
}


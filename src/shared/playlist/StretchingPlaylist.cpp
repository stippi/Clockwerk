/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "StretchingPlaylist.h"

#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"


// constructor 
StretchingPlaylist::StretchingPlaylist(const char* type)
	: Playlist(type ? type : "StretchingPlaylist")
{
}

// constructor 
StretchingPlaylist::StretchingPlaylist(const StretchingPlaylist& other)
	: Playlist(other, true)
{
}

// destructor
StretchingPlaylist::~StretchingPlaylist()
{
}

static inline int64
round_frame(double frame)
{
	return (int64)floor(frame + 0.5);
}

//// ValueChanged
//void
//StretchingPlaylist::ValueChanged(Property* property)
//{
//	Playlist::ValueChanged(property);
//	if (property->Identifier() == PROPERTY_DURATION)
//		ValidateItemLayout();
//}
//
// ValidateItemLayout
void
StretchingPlaylist::ValidateItemLayout()
{
	// make sure we know the correct total duration for the current layout
	Playlist::ValidateItemLayout();

	int64 overriddenDuration = Value(PROPERTY_DURATION, 0LL);
	if (overriddenDuration == 0)
		return;

	double stretchFactor = (double)overriddenDuration / Duration();
	if (stretchFactor == 1.0)
		return;

	// stretch all item locations and property keyframe locations
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);

		if (item->HasMaxDuration()) {
			int64 tooLong = (item->EndFrame() + 1) - overriddenDuration;
			if (tooLong > 0)
				item->SetDuration(item->Duration() - tooLong);
			continue;
		}

		item->SetStartFrame(round_frame(item->StartFrame() * stretchFactor));
		item->SetDuration(round_frame(item->Duration() * stretchFactor));

		int32 propertyCount = item->CountProperties();
		for (int32 j = 0; j < propertyCount; j++) {
			Property* p = item->PropertyAtFast(j);
			PropertyAnimator* animator = p->Animator();
			if (!animator)
				continue;
			int32 keyframeCount = animator->CountKeyFrames();
			for (int32 k = 0; k < keyframeCount; k++) {
				KeyFrame* key = animator->KeyFrameAtFast(k);
				key->SetFrame(round_frame(key->Frame() * stretchFactor));
			}
		}
	}
}

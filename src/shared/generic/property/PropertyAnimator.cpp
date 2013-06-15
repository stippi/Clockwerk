/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PropertyAnimator.h"

#include <new>
#include <stdio.h>

#include <Message.h>

#include "KeyFrame.h"
#include "Property.h"

using std::nothrow;

// constructor
PropertyAnimator::PropertyAnimator(::Property* property)
	: Observable(),
	  fProperty(property),
	  fKeyFrames(20)
{
}

// constructor
PropertyAnimator::PropertyAnimator(::Property* property,
								   const PropertyAnimator& other)
	: Observable(),
	  fProperty(property),
	  fKeyFrames(20)
{
	// clone the keyframes
	int32 count = other.CountKeyFrames();
	for (int32 i = 0; i < count; i++) {
		KeyFrame* copy = new (nothrow) KeyFrame(*other.KeyFrameAtFast(i));
		if (!copy || !fKeyFrames.AddItem((void*)copy)) {
			delete copy;
			fprintf(stderr, "PropertyAnimator() - no memory to"
							"clone the keyframes!\n");
			break;
		}
	}
}

// destructor
PropertyAnimator::~PropertyAnimator()
{
	// NOTE: we don't own fProperty

	// delete the keyframes
	int32 count = CountKeyFrames();
	for (int32 i = 0; i < count; i++)
		delete KeyFrameAtFast(i);
}

// Archive
status_t
PropertyAnimator::Archive(BMessage* into) const
{
	if (!into)
		return B_BAD_VALUE;

	status_t ret = B_OK;

	int32 count = CountKeyFrames();
	for (int32 i = 0; i < count; i++) {
		KeyFrame* key = KeyFrameAtFast(i);
		if (!key->Property())
			continue;

		if (ret == B_OK)
			ret = into->AddInt64("frame", key->Frame());
		if (ret == B_OK)
			ret = into->AddBool("locked", key->IsLocked());
		if (ret == B_OK) {
			BString value;
			key->Property()->GetValue(value);
			ret = into->AddString("value", value.String());
		}

		if (ret < B_OK)
			break;
	}

	return ret;
}

// Unarchive
status_t
PropertyAnimator::Unarchive(const BMessage* archive)
{
	if (!archive)
		return B_BAD_VALUE;

	int64 frame;
	bool locked;
	BString value;
	for (int32 i = 0;
		 archive->FindInt64("frame", i, &frame) == B_OK
		 && archive->FindBool("locked", i, &locked) == B_OK
		 && archive->FindString("value", i, &value) == B_OK; i++) {

		::Property* clone = fProperty->Clone(false);
		if (!clone)
			return B_NO_MEMORY;
		clone->SetValue(value.String());

		KeyFrame* key = new (nothrow) KeyFrame(clone, frame, locked);
		if (!key) {
			delete clone;
			return B_NO_MEMORY;
		}

		if (!AddKeyFrame(key)) {
			delete key;
			return B_NO_MEMORY;
		}
	}

	return B_OK;
}

// DurationChanged
void
PropertyAnimator::DurationChanged(int64 duration)
{
	if (duration <= 0)
		return;

	if (CountKeyFrames() == 0) {
		// add the first keyframe
		AddKeyFrame(new KeyFrame(fProperty->Clone(false), 0, true));
	}

	int64 frame = duration - 1;

	KeyFrame* last = KeyFrameAt(CountKeyFrames() - 1);
	KeyFrame* previous = KeyFrameAt(CountKeyFrames() - 2);
	if (last && previous) {
		// there is a keyframe in front of "last" - get the
		// interpolated property of that one and compare
		// it with the property of the last keyframe
		::Property* temp= previous->Property()->Clone(false);
		if (!temp->SetValue(last->Property())) {
			// the properties have the same values, simply
			// move the last key frame to the clip end
			SetKeyFrameFrame(last, frame);
			Notify();
		}
		delete temp;
	}
}

// SetToFrame
bool
PropertyAnimator::SetToFrame(double frame) const
{
	if (CountKeyFrames() <= 1)
		return false;

	return AnimatePropertyTo(fProperty, frame);
}

// InsertKeyFrameAt
KeyFrame*
PropertyAnimator::InsertKeyFrameAt(int64 frame)
{
	// get the interpolated property at the frame
	::Property* property = fProperty->Clone(false);

	if (!property) {
		fprintf(stderr, "PropertyAnimator::InsertKeyFrameAt() - "
						"no memory to clone property\n");
		return NULL;
	}

	// give the property the correct value
	AnimatePropertyTo(property, frame);

	// create and insert a new keyframe
	KeyFrame* newKey = new (nothrow) KeyFrame(property, frame);
	if (!newKey) {
		fprintf(stderr, "PropertyAnimator::InsertKeyFrameAt() - "
						"no memory for new keyframe\n");
		delete property;
		return NULL;
	}
	if (!AddKeyFrame(newKey)) {
		fprintf(stderr, "PropertyAnimator::InsertKeyFrameAt() - "
						"no memory to insert new keyframe\n");
		delete newKey;
		return NULL;
	}
	return newKey;
}

// SetKeyFrameFrame
void
PropertyAnimator::SetKeyFrameFrame(KeyFrame* key, int64 frame)
{
	if (!key || key->IsLocked() || key->Frame() == frame)
		return;

	int32 index = IndexOf(key);
	if (index < 0)
		return;

	int32 newIndex = _IndexForFrame(frame);
	// account for the key already being contained
	// in the list
	if (newIndex > index)
		newIndex--;

	key->SetFrame(frame);

	if (newIndex == index) {
		Notify();
		return;
	}

	// see if the neighbor keyframe is at the same frame
	// don't change the sorting in this case
	KeyFrame* swapped = KeyFrameAt(newIndex);
	if (swapped && swapped->Frame() == frame)
		return;

	// move arround item pointers to keep
	// the keyframes sorted by frame
	KeyFrame** items = (KeyFrame**)fKeyFrames.Items();
	if (newIndex < index) {
		// move up
		int32 count = index - newIndex;
		items += index;
		for (int32 i = 0; i < count; i++) {
			items[0] = items[-1];
			items--;
		}
		items[0] = key;
	} else {
		// move down
		int32 count = newIndex - index;
		items += index;
		for (int32 i = 0; i < count; i++) {
			items[0] = items[1];
			items++;
		}
		items[0] = key;
	}
	Notify();
}

// #pragma mark -

// AddKeyFrame
bool
PropertyAnimator::AddKeyFrame(KeyFrame* key)
{
	if (!key)
		return false;

	int32 index = _IndexForFrame(key->Frame());

	if (fKeyFrames.AddItem((void*)key, index)) {
		Notify();
		return true;
	}
	return false;
}

// RemoveKeyFrame
KeyFrame*
PropertyAnimator::RemoveKeyFrame(int32 index)
{
	KeyFrame* key = (KeyFrame*)fKeyFrames.RemoveItem(index);
	if (key) {
		Notify();
	}
	return key;
}

// RemoveKeyFrame
bool
PropertyAnimator::RemoveKeyFrame(KeyFrame* key)
{
	if (fKeyFrames.RemoveItem((void*)key)) {
		Notify();
		return true;
	}
	return false;
}

// KeyFrameAt
KeyFrame*
PropertyAnimator::KeyFrameAt(int32 index) const
{
	return (KeyFrame*)fKeyFrames.ItemAt(index);
}

// KeyFrameAtFast
KeyFrame*
PropertyAnimator::KeyFrameAtFast(int32 index) const
{
	return (KeyFrame*)fKeyFrames.ItemAtFast(index);
}

// HasKeyFrame
bool
PropertyAnimator::HasKeyFrame(KeyFrame* key) const
{
	return fKeyFrames.HasItem((void*)key);
}

// CountKeyFrames
int32
PropertyAnimator::CountKeyFrames() const
{
	return fKeyFrames.CountItems();
}

// IndexOf
int32
PropertyAnimator::IndexOf(KeyFrame* key) const
{
	return fKeyFrames.IndexOf((void*)key);
}

// MakeEmpty
void
PropertyAnimator::MakeEmpty()
{
	int32 count = fKeyFrames.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (KeyFrame*)fKeyFrames.ItemAtFast(i);
	fKeyFrames.MakeEmpty();
	Notify();
}

// #pragma mark -

// KeyFrameBeforeOrAt
KeyFrame*
PropertyAnimator::KeyFrameBeforeOrAt(int64 frame) const
{
	int32 index = max_c(0, _IndexForFrame(frame) - 1);
	return KeyFrameAt(index);
}

// KeyFrameBehind
KeyFrame*
PropertyAnimator::KeyFrameBehind(int64 frame) const
{
	// TODO: not sure about this one...
	int32 index = _IndexForFrame(frame);
	return KeyFrameAt(index);
}

// ScaleAt
float
PropertyAnimator::ScaleAt(int64 frame) const
{
	KeyFrame* before = KeyFrameBeforeOrAt(frame);
	if (before && before->Frame() == frame)
		return before->Scale();

	KeyFrame* after = KeyFrameBehind(frame);
	if (before && !after)
		return before->Scale();
	else if (!before && after)
		return after->Scale();
	else if (before && after) {
		// interpolate scale
		float scaleDiff = after->Scale() - before->Scale();
		int64 frameDiff = after->Frame() - before->Frame();
		frame -= before->Frame();
		return before->Scale() + scaleDiff * (frame / (float)frameDiff);
	}

	return 1.0;
}

// ScaleAtFloat
float
PropertyAnimator::ScaleAtFloat(float frame) const
{
	int64 videoFrame = (int64)frame;
	KeyFrame* before = KeyFrameBeforeOrAt(videoFrame);
	if (before && before->Frame() == frame)
		return before->Scale();

	KeyFrame* after = KeyFrameBehind(videoFrame);
	if (before && !after)
		return before->Scale();
	else if (!before && after)
		return after->Scale();
	else if (before && after) {
		// interpolate scale
		float scaleDiff = after->Scale() - before->Scale();
		float frameDiff = after->Frame() - before->Frame();
		frame -= before->Frame();
		return before->Scale() + scaleDiff * (frame / frameDiff);
	}

	return 1.0;
}

// AnimatePropertyTo
bool
PropertyAnimator::AnimatePropertyTo(::Property* property,
									 double frame) const
{
	// find closest index
	int32 index = _IndexForFrame((int64)frame);

	// this is an insert index, so decrement it
	index = max_c(0, index - 1);

	// there must be a keyframe at the index
	// and we derive a property from that
	KeyFrame* previousKey = KeyFrameAt(index);
	if (!previousKey)
		return false;

	bool changed = property->SetValue(previousKey->Property());

	if (previousKey->Frame() >= frame)
		return changed;

	// if there is a "next" keyframe, we can interpolate
	// the property
	KeyFrame* nextKey = KeyFrameAt(index + 1);
	if (nextKey) {
		float diff = (float)(frame - previousKey->Frame());
		float total = (float)(nextKey->Frame() - previousKey->Frame());
		float interpolationScale = diff / total;
		return property->InterpolateTo(nextKey->Property(),
									   interpolationScale) || changed;
	}
	return changed;
}

// #pragma mark -

// _IndexForFrame
int32
PropertyAnimator::_IndexForFrame(int64 frame) const
{
//printf("_IndexForFrame(%lld)\n", frame);
	// find the KeyFrame before or at frame
	// if there are no KeyFrames earlier than frame,
	// return 0

	// binary search
	int32 lower = 0;
	int32 upper = CountKeyFrames();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
//printf("  lower: %ld, upper: %ld -> mid: %ld\n", lower, upper, mid);
		int64 midFrame = KeyFrameAtFast(mid)->Frame();
		if (frame < midFrame)
			// NOTE: "frame < midFrame"
			// in order to insert new keyframes behind
			// existing keyframes in case their are on the
			// same frame
			upper = mid;
		else
			lower = mid + 1;
	}
//printf("  result: %ld\n", lower);
//	// we want to support more than one keyframe
//	// at the same frame, but it is important to
//	// return the last one
//	KeyFrame* key = KeyFrameAt(lower);
//	KeyFrame* nextKey = KeyFrameAt(lower + 1);
//	while (nextKey && nextKey->Frame() == key->Frame()) {
//		// NOTE: it is not possible to have
//		// "nextKey != NULL && key == NULL"
//		lower ++;
//		nextKey = KeyFrameAt(lower + 1);
//	}
//
	return lower;
}

//// _CleanUp
//void
//PropertyAnimator::_CleanUp(int64 fromOffset, int64 toOffset)
//{
//	// find start and stop indices
//	int32 startIndex = _IndexForFrame(fromOffset);
//	int32 stopIndex = _IndexForFrame(toOffset);
//
//	// these are insertion indices, so decrement them
//	startIndex = max_c(0, startIndex - 1);
//	stopIndex = max_c(0, stopIndex - 1);
//
//	KeyFrame* previous = KeyFrameAt(startIndex);
//
//	for (int32 i = startIndex + 1; i <= stopIndex; i++) {
//		KeyFrame* key = KeyFrameAt(i);
//		XXXX
//	}
//}

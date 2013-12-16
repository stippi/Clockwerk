/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistItem.h"

#include <new>
#include <stdio.h>

#include <Message.h>

#include "AdvancedTransform.h"
#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "Playlist.h"
#include "Property.h"
#include "PropertyAnimator.h"

using std::nothrow;

// constructor 
PlaylistItem::PlaylistItem(int64 startFrame,
						   uint64 duration,
						   uint32 track)
	: PropertyObject()
	, Selectable()

	, fParent(NULL)
	, fStartFrame(startFrame)
	, fDuration(duration)
	, fClipOffset(0)
	, fTrack(track)

	, fVideoMuted(false)
	, fAudioMuted(false)
{
	_CreateProperties();
}

// constructor 
PlaylistItem::PlaylistItem(const PlaylistItem& other, bool deep)
	: PropertyObject(other, deep)
	, Selectable()
	, fParent(NULL) // NOTE: NULL on purpose
	, fStartFrame(other.fStartFrame)
	, fDuration(other.fDuration)
	, fClipOffset(other.fClipOffset)
	, fTrack(other.fTrack)
	, fAlpha(FindFloatProperty(PROPERTY_OPACITY))

	, fPivotX(FindFloatProperty(PROPERTY_PIVOT_X))
	, fPivotY(FindFloatProperty(PROPERTY_PIVOT_Y))
	, fTranslationX(FindFloatProperty(PROPERTY_TRANSLATION_X))
	, fTranslationY(FindFloatProperty(PROPERTY_TRANSLATION_Y))
	, fRotation(FindFloatProperty(PROPERTY_ROTATION))
	, fScaleX(FindFloatProperty(PROPERTY_SCALE_X))
	, fScaleY(FindFloatProperty(PROPERTY_SCALE_Y))

	, fVideoMuted(other.fVideoMuted)
	, fAudioMuted(other.fAudioMuted)
{
}

// constructor 
PlaylistItem::PlaylistItem(BMessage* archive)
	: PropertyObject()
	, Selectable()

	, fParent(NULL)
	, fStartFrame(0)
	, fDuration(0)
	, fClipOffset(0)
	, fTrack(0)

	, fVideoMuted(false)
	, fAudioMuted(false)
{
	_CreateProperties();

	if (!archive)
		return;

	// restore properties
	PropertyObject::Unarchive(archive);

	// restore members
	if (archive->FindInt64("start_frame", &fStartFrame) < B_OK)
		fStartFrame = 0;
	if (archive->FindInt64("duration", (int64*)&fDuration) < B_OK)
		fDuration = 0;
	if (archive->FindInt64("clip_offset", (int64*)&fClipOffset) < B_OK)
		fClipOffset = 0;
	if (archive->FindInt32("track", (int32*)&fTrack) < B_OK)
		fTrack = 0;
	if (archive->FindBool("video_muted", &fVideoMuted) < B_OK)
		fVideoMuted = false;
	if (archive->FindBool("audio_muted", &fAudioMuted) < B_OK)
		fAudioMuted = false;
}

// destructor
PlaylistItem::~PlaylistItem()
{
}

// SelectedChanged
void
PlaylistItem::SelectedChanged()
{
	Notify();
}

// Archive
status_t
PlaylistItem::Archive(BMessage* into, bool deep) const
{
	// store properties
	status_t ret = PropertyObject::Archive(into);
	if (ret < B_OK)
		printf("PropertyObject::Archive(): %s\n", strerror(ret));

	// store members
	if (ret == B_OK)
		ret = into->AddInt64("start_frame", fStartFrame);
	if (ret == B_OK)
		ret = into->AddInt64("duration", fDuration);
	if (ret == B_OK)
		ret = into->AddInt64("clip_offset", fClipOffset);
	if (ret == B_OK)
		ret = into->AddInt32("track", fTrack);
	if (ret == B_OK)
		ret = into->AddBool("video_muted", fVideoMuted);
	if (ret == B_OK)
		ret = into->AddBool("audio_muted", fAudioMuted);

	// NOTE: we don't finish with class name,
	// since we are not directly instantiatable

	if (ret < B_OK)
		printf("PlaylistItem::Archive(): %s\n", strerror(ret));

	return ret;
}

// ResolveDependencies
status_t
PlaylistItem::ResolveDependencies(const ServerObjectManager* library)
{
	return B_OK;
}

//// SetTo
//bool
//PlaylistItem::SetTo(const PlaylistItem* other)
//{
//	// NOTE: fParent is not cloned!
//	fStartFrame = other->fStartFrame;
//	fDuration = other->fDuration;
//	fClipOffset = other->fClipOffset;
//	fTrack = other->fTrack;
//
//	// TODO: re-init properties
//
//	return true;
//}

// SetCurrentFrame
void
PlaylistItem::SetCurrentFrame(double frame)
{
	bool notify = false;
	double localFrame = (frame - StartFrame()) + ClipOffset();
	int32 count = CountProperties();
	for (int32 i = 0; i < count; i++) {
		PropertyAnimator* animator = PropertyAtFast(i)->Animator();
		if (animator && animator->SetToFrame(localFrame))
			notify = true;
	}
	if (notify)
		Notify();
}

// SetParent
void
PlaylistItem::SetParent(Playlist* parent)
{
	if (fParent != parent) {
		fParent = parent;
		// stuff like transformation might have changed
		Notify();
	}
}

// SetVideoMuted
void
PlaylistItem::SetVideoMuted(bool muted)
{
	if (fVideoMuted != muted) {
		fVideoMuted = muted;
		Notify();
	}
}

// SetAudioMuted
void
PlaylistItem::SetAudioMuted(bool muted)
{
	if (fAudioMuted != muted) {
		fAudioMuted = muted;
		Notify();
	}
}

// MouseDown
bool
PlaylistItem::MouseDown(BPoint where, uint32 buttons, BRect canvasBounds,
	double frame)
{
	return false;
}

// SetStartFrame
void
PlaylistItem::SetStartFrame(int64 startFrame)
{
	startFrame -= fClipOffset;
	if (fStartFrame != startFrame) {
		fStartFrame = startFrame;
		Notify();
	}
}

// SetDuration
void
PlaylistItem::SetDuration(uint64 duration)
{
	duration += fClipOffset;
	if (fDuration != duration) {
		fDuration = duration;

		// keep the animators informed
		int32 count = CountProperties();
		for (int32 i = 0; i < count; i++) {
			PropertyAnimator* animator = PropertyAtFast(i)->Animator();
			if (animator)
				animator->DurationChanged(fDuration);
		}

		Notify();

//		// TODO: it is not that good to
//		// have this here (it is not in SetStartFrame()!)
//		if (fParent)
//			fParent->ItemsChanged();
	}
}

// MaxDuration
uint64
PlaylistItem::MaxDuration() const
{
	return LONG_MAX;
}

// HasMaxDuration
bool
PlaylistItem::HasMaxDuration() const
{
	return MaxDuration() < LONG_MAX;
}

// SetClipOffset
void
PlaylistItem::SetClipOffset(uint64 offset)
{
	if (offset > fDuration - 1)
		offset = fDuration - 1;

	if (fClipOffset != offset) {
		fClipOffset = offset;
		Notify();
	}
}

// SetTrack
void
PlaylistItem::SetTrack(uint32 track)
{
	if (fTrack != track) {
		fTrack = track;
		Notify();
	}
}


// ConvertFrameToLocal
void
PlaylistItem::ConvertFrameToLocal(int64& frame) const
{
	frame -= StartFrame();
}

// VideoFramesPerSecond
float
PlaylistItem::VideoFramesPerSecond() const
{
	if (fParent)
		return fParent->VideoFrameRate();
	return 25.0;
}

// #pragma mark -

// Transformation
AffineTransform
PlaylistItem::Transformation() const
{
	AdvancedTransform transform;
	if (fPivotX && fPivotY
		&& fTranslationX && fTranslationY
		&& fRotation
		&& fScaleX && fScaleY) {
		transform.SetTransformation(BPoint(fPivotX->Value(), fPivotY->Value()),
			BPoint(fTranslationX->Value(), fTranslationY->Value()),
			fRotation->Value(), fScaleX->Value(), fScaleY->Value());
	}
	return transform;
}

// Alpha
float
PlaylistItem::Alpha() const
{
	return fAlpha ? fAlpha->Value() : 1.0;
}

// AlphaAnimator
PropertyAnimator*
PlaylistItem::AlphaAnimator() const
{
	return fAlpha ? fAlpha->Animator() : NULL;
}

// #pragma mark -

// protected copy constructor
PlaylistItem::PlaylistItem(const PlaylistItem& other)
	: PropertyObject() // cloning properties not desired

	, fParent(NULL)

	, fStartFrame(other.fStartFrame)
	, fDuration(other.fDuration)
	, fClipOffset(other.fClipOffset)
	, fTrack(other.fTrack)

	, fAlpha(NULL)

	, fPivotX(NULL)
	, fPivotY(NULL)
	, fTranslationX(NULL)
	, fTranslationY(NULL)
	, fRotation(NULL)
	, fScaleX(NULL)
	, fScaleY(NULL)

	, fVideoMuted(other.fVideoMuted)
	, fAudioMuted(other.fAudioMuted)
{
}

// _CreateProperties
void
PlaylistItem::_CreateProperties()
{
	fAlpha = new (nothrow) FloatProperty(PROPERTY_OPACITY, 1.0, 0.0, 1.0);

	fPivotX = new (nothrow) FloatProperty(PROPERTY_PIVOT_X, 0.0);
	fPivotY = new (nothrow) FloatProperty(PROPERTY_PIVOT_Y, 0.0);
	fTranslationX = new (nothrow) FloatProperty(PROPERTY_TRANSLATION_X, 0.0);
	fTranslationY = new (nothrow) FloatProperty(PROPERTY_TRANSLATION_Y, 0.0);
	fRotation = new (nothrow) FloatProperty(PROPERTY_ROTATION, 0.0);
	fScaleX = new (nothrow) FloatProperty(PROPERTY_SCALE_X, 1.0);
	fScaleY = new (nothrow) FloatProperty(PROPERTY_SCALE_Y, 1.0);

	if (!fAlpha) {
		printf("PlaylistItem::_CreateProperties() - "
			   "no memory to create alpha property\n");
		return;
	}

	AddProperty(fAlpha);

	fAlpha->MakeAnimatable();
	if (PropertyAnimator* animator = fAlpha->Animator())
		animator->DurationChanged(Duration());

	AddProperty(fPivotX);
	AddProperty(fPivotY);
	AddProperty(fTranslationX);
	AddProperty(fTranslationY);
	AddProperty(fRotation);
	AddProperty(fScaleX);
	AddProperty(fScaleY);
}

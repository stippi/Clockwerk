/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipPlaylistItem.h"

#include <new>

#include <stdio.h>

#include <Message.h>

#include "common.h"

#include "Clip.h"
#include "CommonPropertyIDs.h"
#include "PlaybackNavigator.h"
#include "Playlist.h"
#include "PlaylistItemAudioReader.h"
#include "Property.h"
#include "ServerObjectManager.h"

using std::nothrow;

// constructor
ClipPlaylistItem::ClipPlaylistItem(::Clip* clip, int64 startFrame, uint32 track)
	: PlaylistItem(startFrame, 0, track)
	, fClip(NULL)
{
	SetClip(clip);

	// maximum automatic gain for audio playback
	AddProperty(new (nothrow) FloatProperty(PROPERTY_MAX_AUTO_GAIN,
		1.0, 1.0, 32.0));
}

// constructor 
ClipPlaylistItem::ClipPlaylistItem(const ClipPlaylistItem& other, bool deep)
	: PlaylistItem(other, deep)
	, fClip(other.fClip)
{
	if (fClip) {
		fClip->Acquire();
		fClip->AddObserver(this);
	}
}

// constructor
ClipPlaylistItem::ClipPlaylistItem(BMessage* archive)
	: PlaylistItem(archive)
	, fClip(NULL)
{
	if (!archive)
		return;

	const char* clipID;
	if (archive->FindString("clip_id", &clipID) == B_OK)
		AddProperty(new (nothrow) StringProperty(PROPERTY_CLIP_ID, clipID));
}

// destructor
ClipPlaylistItem::~ClipPlaylistItem()
{
	SetClip(NULL);
}

// Instantiate
BArchivable*
ClipPlaylistItem::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ClipPlaylistItem"))
		return NULL;

	return new (nothrow) ClipPlaylistItem(archive);
}

// Archive
status_t
ClipPlaylistItem::Archive(BMessage* into, bool deep) const
{
	// store base class
	status_t ret = PlaylistItem::Archive(into);

	if (ret == B_OK) {
		if (fClip) {
			ret = into->AddString("clip_id", fClip->ID());
		} else {
			StringProperty* s = dynamic_cast<StringProperty*>(
				FindProperty(PROPERTY_CLIP_ID));
			if (s)
				ret = into->AddString("clip_id", s->Value());
		}
	}

	// finish with class name
	if (ret == B_OK)
		ret = into->AddString("class", "ClipPlaylistItem");

	if (ret < B_OK)
		print_error("ClipPlaylistItem::Archive(): %s\n", strerror(ret));

	return ret;
}

// ResolveDependencies
status_t
ClipPlaylistItem::ResolveDependencies(const ServerObjectManager* library)
{
	if (fClip)
		return B_OK;

	StringProperty* s = dynamic_cast<StringProperty*>(
		FindProperty(PROPERTY_CLIP_ID));
	if (!s) {
		print_error("##### ClipPlaylistItem::ResolveDependencies() - "
			"didn't find clip id property\n");
		return B_ERROR;
	}

	::Clip* clip = dynamic_cast< ::Clip*>(library->FindObject(s->Value()));
	if (!clip) {
// NOTE: this is actually not an error anymore. Truely missing dependencies are
// reported by the Controller, with much better info (like what clip missing
// by what other clip...) Missing clips happen now on purpose for out of date
// CollectablePlaylists
//		print_error("##### ClipPlaylistItem::ResolveDependencies() - "
//			"didn't find clip: %s\n", s->Value());
		return B_ERROR;
	}

	if (!clip->IsValid()) {
		// this clip has not resolved it's own dependencies yet,
		// which might screw up our "max duration" calculation in
		// SetClip().
		status_t ret = clip->ResolveDependencies(library);
		clip->SetDependenciesResolved(ret == B_OK);
	}

	SetClip(clip);

	return B_OK;
}

// Clone
PlaylistItem*
ClipPlaylistItem::Clone(bool deep) const
{
	return new (nothrow) ClipPlaylistItem(*this, deep);
}

// HasAudio
bool
ClipPlaylistItem::HasAudio()
{
	return fClip && fClip->HasAudio();
}

// CreateAudioReader
AudioReader*
ClipPlaylistItem::CreateAudioReader()
{
	if (!fClip)
		return NULL;

	AudioReader* reader = fClip->CreateAudioReader();
	if (!reader) {
		printf("ClipPlaylistItem::CreateAudioReader() - MediaClip is supposed "
			"to create AudioReader, but failed to do so!\n");
		return NULL;
	}

	return new PlaylistItemAudioReader(this, reader);
}

// HasVideo
bool
ClipPlaylistItem::HasVideo()
{
	return fClip && fClip->HasVideo();
}

// Bounds
BRect
ClipPlaylistItem::Bounds(BRect canvasBounds, bool transformed)
{
	if (!fClip)
		return BRect(0, 0, -1, -1);

	BRect bounds = fClip->Bounds(canvasBounds);
	if (transformed)
		return Transformation().TransformBounds(bounds);
	return bounds;
}

// Name
BString
ClipPlaylistItem::Name() const
{
	if (!fClip)
		return BString("<no clip>");

	return fClip->Name();
}

// MouseDown
bool
ClipPlaylistItem::MouseDown(BPoint where, uint32 buttons, BRect canvasBounds,
	double frame, PlaybackNavigator* navigator)
{
//printf("%p->ClipPlaylistItem::MouseDown(BPoint(%.1f, %.1f), %ld, "
//	"BRect(%.1f, %.1f, %.1f, %.1f), %p)\n", this, where.x, where.y,
//	buttons, canvasBounds.left, canvasBounds.top, canvasBounds.right,
//	canvasBounds.bottom, navigator);

	Playlist* playlist = dynamic_cast<Playlist*>(fClip);
	if (playlist) {
		AffineTransform transform = Transformation();
		BPoint transformedWhere = transform.Transform(where);
		return playlist->MouseDown(transformedWhere, buttons, canvasBounds,
			frame, navigator);
	}

	const ::NavigationInfo* info = NavigationInfo();
	if (!info || !navigator)
		return false;

	navigator->Navigate(info);
	return true;
}

// MaxDuration
uint64
ClipPlaylistItem::MaxDuration() const
{
	if (!fClip)
		return PlaylistItem::MaxDuration();

	return fClip->MaxDuration();
}

// #pragma mark -

// disallowed constructor
ClipPlaylistItem::ClipPlaylistItem(const ClipPlaylistItem& other)
{
}

// ObjectChanged
void
ClipPlaylistItem::ObjectChanged(const Observable* object)
{
	if (object == fClip) {

		uint64 duration = fClip->Duration();
		if (Duration() == 0) {
			SetDuration(duration);
		} else {
			// pass on the event
			Notify();
		}
	} else {
		// debugging
		debugger("ClipPlaylistItem::ObjectChanged() - "
				 "received notification for the wrong object\n");
	}
}

// SetClip
void
ClipPlaylistItem::SetClip(::Clip* clip)
{
	if (fClip == clip)
		return;

	AutoNotificationSuspender _(this);

	if (fClip) {
		fClip->RemoveObserver(this);
		fClip->Release();
			// NOTE: clip might have selfdestroyed now
	}

	fClip = clip;

	if (fClip) {
		fClip->Acquire();
		fClip->AddObserver(this);

		BRect canvas(0.0, 0.0, -1.0, -1.0);
		BRect bounds = fClip->Bounds(canvas);
		if (bounds.IsValid()) {
			SetValue(PROPERTY_PIVOT_X,
				(float)((bounds.left + bounds.right) / 2.0));
			SetValue(PROPERTY_PIVOT_Y,
				(float)((bounds.top + bounds.bottom) / 2.0));
		}

		if (Duration() == 0)
			SetDuration(DefaultDuration(fClip->Duration()));
		else if (Duration() > fClip->MaxDuration())
			SetDuration(fClip->MaxDuration());

		// if this property existed, we no longer need it
		DeleteProperty(PROPERTY_CLIP_ID);
	}

	Notify();
}

// DefaultDuration
uint64
ClipPlaylistItem::DefaultDuration(uint64 duration)
{
	if (duration == 0)
		return 100;
	return duration;
}

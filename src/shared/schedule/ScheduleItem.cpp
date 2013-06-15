/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleItem.h"

#include <new>
#include <stdio.h>

#include <Message.h>

#include "common.h"

#include "CommonPropertyIDs.h"
#include "Playlist.h"
#include "Property.h"
#include "Schedule.h"
#include "ServerObjectManager.h"
#include "TimeProperty.h"

using std::nothrow;

// constructor 
ScheduleItem::ScheduleItem(::Playlist* playlist)
	: PropertyObject()
	, Selectable()

	, fParent(NULL)
	, fPlaylist(NULL)

	, fStartFrame(0)
	, fDuration(0)
	, fExplicitRepeats(0)

	, fFlexibleStartFrame(true)
	, fFlexibleDuration(false)
{
	SetPlaylist(playlist);
	SetDuration(_PlaylistDuration());
}

// constructor 
ScheduleItem::ScheduleItem(const ScheduleItem& other, bool deep)
	: PropertyObject(other, deep)
	, Selectable()

	, fParent(NULL) // NOTE: NULL on purpose
	, fPlaylist(NULL)

	, fStartFrame(other.fStartFrame)
	, fDuration(other.fDuration)
	, fExplicitRepeats(other.fExplicitRepeats)

	, fFlexibleStartFrame(other.fFlexibleStartFrame)
	, fFlexibleDuration(other.fFlexibleDuration)
{
	SetPlaylist(other.fPlaylist);
}

// constructor 
ScheduleItem::ScheduleItem(BMessage* archive)
	: PropertyObject()
	, Selectable()

	, fParent(NULL)
	, fPlaylist(NULL)

	, fStartFrame(0)
	, fDuration(0)
	, fExplicitRepeats(0)

	, fFlexibleStartFrame(true)
	, fFlexibleDuration(false)
{
	if (!archive)
		return;

	const char* playlistID;
	if (archive->FindString("playlist_id", &playlistID) == B_OK)
		AddProperty(new (nothrow) StringProperty(PROPERTY_CLIP_ID, playlistID));

	if (archive->FindInt64("startframe", (int64*)&fStartFrame) < B_OK)
		fStartFrame = 0;
	if (archive->FindInt64("duration", (int64*)&fDuration) < B_OK)
		fDuration = 0;
	if (archive->FindInt16("repeats", (int16*)&fExplicitRepeats) < B_OK)
		fExplicitRepeats = 0;

	bool option;
	if (archive->FindBool("flexible startframe", &option) == B_OK)
		fFlexibleStartFrame = option;
	if (archive->FindBool("flexible duration", &option) == B_OK)
		fFlexibleDuration = option;

	// restore properties
	PropertyObject::Unarchive(archive);
}

// destructor
ScheduleItem::~ScheduleItem()
{
	SetPlaylist(NULL);
}

// SelectedChanged
void
ScheduleItem::SelectedChanged()
{
	Notify();
}

// Instantiate
/*static*/ BArchivable*
ScheduleItem::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "ScheduleItem"))
		return NULL;

	return new (nothrow) ScheduleItem(archive);
}

// DurationChanged (PlaylistObserver)
void
ScheduleItem::DurationChanged(::Playlist* playlist, uint64 duration)
{
	_AdjustDuration();

	// TODO: these notifications can amount to quite a bit of work,
	// they should not do unnecessary work
	if (fParent)
		fParent->SanitizeStartFrames();
}

// Archive
status_t
ScheduleItem::Archive(BMessage* into, bool deep) const
{
	// store properties
	status_t ret = PropertyObject::Archive(into);
	if (ret < B_OK)
		printf("PropertyObject::Archive(): %s\n", strerror(ret));

	if (ret == B_OK) {
		if (fPlaylist) {
			ret = into->AddString("playlist_id", fPlaylist->ID());
		} else {
			StringProperty* s = dynamic_cast<StringProperty*>(
				FindProperty(PROPERTY_CLIP_ID));
			if (s)
				ret = into->AddString("playlist_id", s->Value());
		}
	}

	if (ret == B_OK)
		ret = into->AddInt64("startframe", fStartFrame);
	if (ret == B_OK)
		ret = into->AddInt64("duration", fDuration);
	if (ret == B_OK)
		ret = into->AddInt16("repeats", fExplicitRepeats);

	if (ret == B_OK)
		ret = into->AddBool("flexible startframe", fFlexibleStartFrame);
	if (ret == B_OK)
		ret = into->AddBool("flexible duration", fFlexibleDuration);

	// finish with class name
	if (ret == B_OK)
		ret = into->AddString("class", "ScheduleItem");

	if (ret < B_OK)
		print_error("ScheduleItem::Archive(): %s\n", strerror(ret));

	return ret;
}

// ResolveDependencies
status_t
ScheduleItem::ResolveDependencies(const ServerObjectManager* library)
{
	if (fPlaylist)
		return B_OK;

	StringProperty* s = dynamic_cast<StringProperty*>(
		FindProperty(PROPERTY_CLIP_ID));
	if (!s) {
		// means this is an "empty on purpose" schedule item
		return B_OK;
	}

	::Playlist* playlist = dynamic_cast< ::Playlist*>(library->FindObject(s->Value()));
	if (!playlist) {
//		print_error("ScheduleItem::ResolveDependencies() - "
//			"didn't find clip: %s\n", s->Value());
		return B_ERROR;
	}

	SetPlaylist(playlist);

	return B_OK;
}

// Clone
ScheduleItem*
ScheduleItem::Clone(bool deep) const
{
	return new (nothrow) ScheduleItem(*this, deep);
}

// Name
BString
ScheduleItem::Name() const
{
	if (!fPlaylist)
		return BString("<no playlist>");

	return fPlaylist->Name();
}

// SetParent
void
ScheduleItem::SetParent(Schedule* parent)
{
	fParent = parent;
}

// SetPlaylist
void
ScheduleItem::SetPlaylist(::Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	AutoNotificationSuspender _(this);

	if (fPlaylist) {
		fPlaylist->RemoveListObserver(this);
		fPlaylist->Release();
			// NOTE: playlist might have selfdestroyed now
	}

	fPlaylist = playlist;

	if (fPlaylist) {
		fPlaylist->Acquire();
		fPlaylist->AddListObserver(this);

		// if this property existed, we no longer need it
		DeleteProperty(PROPERTY_CLIP_ID);

		_AdjustDuration();
	}

	Notify();
}

// SetStartFrame
void
ScheduleItem::SetStartFrame(uint64 frameOfDay)
{
	if (fStartFrame == frameOfDay)
		return;

	fStartFrame = frameOfDay;
	Notify();
}

// SetDuration
void
ScheduleItem::SetDuration(uint64 frames)
{
	if (fDuration == frames)
		return;

	fDuration = frames;
	Notify();
}

// PreferredDuration
uint64
ScheduleItem::PreferredDuration() const
{
	uint64 playlistDuration = _PlaylistDuration();
	if (playlistDuration > 0)
		return playlistDuration * (fExplicitRepeats + 1);
	return fDuration;
}

// PlaylistDuration
uint64
ScheduleItem::PlaylistDuration() const
{
	return _PlaylistDuration();
}

// SetExplicitRepeats
void
ScheduleItem::SetExplicitRepeats(uint16 repeats)
{
	if (fExplicitRepeats == repeats)
		return;

	fExplicitRepeats = repeats;

	AutoNotificationSuspender _(this);
	_AdjustDuration();
	Notify();
}

// Repeats
float
ScheduleItem::Repeats() const
{
	uint64 playlistDuration = _PlaylistDuration();
	if (playlistDuration == 0)
		return 0.0;
	return (float)fDuration / playlistDuration - 1;
}

// #pragma mark -

// SetFlexibleStartFrame
void
ScheduleItem::SetFlexibleStartFrame(bool flexible)
{
	if (fFlexibleStartFrame == flexible)
		return;

	fFlexibleStartFrame = flexible;

	Notify();
}

// SetFlexibleDuration
void
ScheduleItem::SetFlexibleDuration(bool flexible)
{
	if (fFlexibleDuration == flexible)
		return;

	fFlexibleDuration = flexible;

	AutoNotificationSuspender _(this);
	_AdjustDuration();
	Notify();
}

// #pragma mark -

// FilterStartFrame
void
ScheduleItem::FilterStartFrame(uint64* frameOfDay) const
{
	if (fFlexibleStartFrame)
		return;

	*frameOfDay = fStartFrame;
}

// FilterDuration
void
ScheduleItem::FilterDuration(uint64* duration) const
{
	*duration = max_c(1, *duration);

	if (!fPlaylist)
		return;

	if (fFlexibleDuration)
		return;

	*duration = PreferredDuration();
}

// #pragma mark -

// disabled copy constructor
ScheduleItem::ScheduleItem(const ScheduleItem& other)
	: PropertyObject() // cloning properties not desired

	, fParent(NULL)
	, fPlaylist(NULL)

	, fStartFrame(0)
	, fDuration(0)
	, fExplicitRepeats(0)

	, fFlexibleStartFrame(true)
	, fFlexibleDuration(false)
{
}

// _CreateProperties
void
ScheduleItem::_CreateProperties()
{
}

// _PlaylistDuration
uint64
ScheduleItem::_PlaylistDuration() const
{
	if (!fPlaylist)
		return 0;

	return fPlaylist->Duration();
}

// _AdjustDuration
void
ScheduleItem::_AdjustDuration()
{
	uint64 duration = fDuration;
	FilterDuration(&duration);
	SetDuration(duration);
}

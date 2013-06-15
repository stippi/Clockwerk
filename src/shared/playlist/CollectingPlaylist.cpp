/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CollectingPlaylist.h"

#include <new>
#include <time.h>
#include <parsedate.h>
#include <stdio.h>
#include <stdlib.h>

#include "common.h"

#include "CollectablePlaylist.h"
#include "CommonPropertyIDs.h"
#include "Clip.h"
#include "ClipPlaylistItem.h"
#include "KeyFrame.h"
#include "OptionProperty.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "ServerObjectManager.h"


using std::nothrow;

// constructor 
CollectingPlaylist::CollectingPlaylist()
	: Playlist("CollectingPlaylist")
{
}

// constructor 
CollectingPlaylist::CollectingPlaylist(const CollectingPlaylist& other)
	: Playlist(other, true)
{
}

// destructor
CollectingPlaylist::~CollectingPlaylist()
{
}

// #pragma mark -

//// MaxDuration
//uint64
//CollectingPlaylist::MaxDuration()
//{
//	return (uint64)Value(PROPERTY_DURATION, (int64)60 * 60 * 25);
//}


// ResolveDependencies
status_t
CollectingPlaylist::ResolveDependencies(const ServerObjectManager* library)
{
//uint64 oldDuration = Duration();
	MakeEmpty();

	// find the type marker, so we know which collectable playlists
	// we are looking for
	BString typeMarker(TypeMarker());
	if (typeMarker.Length() <= 0) {
		print_error("CollectingPlaylist::ResolveDependencies() - "
			"no \"type marker\" specified!\n");
		return B_ERROR;
	}

	// * find all TemplatePlaylists with a certain type, and which
	//   are valid for "today"
	// * add ClipPlaylistItems for each above created clip and
	//   layout these, separating individual sequences by the
	//   transitionClip
	BList collectables;

	int32 count = library->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = library->ObjectAtFast(i);
		CollectablePlaylist* collectable = dynamic_cast<CollectablePlaylist*>(
			object);
		if (!collectable)
			continue;

		if (typeMarker != collectable->TypeMarker()) {
//print_info("found Collectable playlist, but type does not match: %s/%s\n",
//	typeMarker.String(), collectable->TypeMarker());
			continue;
		}

		OptionProperty* statusOption = dynamic_cast<OptionProperty*>(
			collectable->FindProperty(PROPERTY_STATUS));
		// in case the object does have this property, check
		// the status and don't collect referenced objects if
		// it is not set to "published"
		if (statusOption
			&& statusOption->Value() != PLAYLIST_STATUS_LIVE) {
			continue;
		}

		if (!collectable->IsValidToday()) {
//print_info("found fitting Collectable playlist, but it is outdated: %s/%ld\n",
//	collectable->StartDate(), collectable->ValidDays());
			continue;
		}

		if (collectables.AddItem(collectable)) {
		} else {
			print_error("CollectingPlaylist::ResolveDependencies() - "
				"no memory to add collectable playlist\n");
			return B_NO_MEMORY;
		}
	}

	if (collectables.CountItems() <= 0)
		return B_OK;

	status_t ret;
	switch (CollectorMode()) {
		case COLLECTOR_MODE_RANDOM:
			ret = _CollectRandom(collectables, library);
			break;
		case COLLECTOR_MODE_SEQUENCE:
		default:
			ret = _CollectSequence(collectables, library);
			break;
	}

	Playlist::_ItemsChanged();

	_LayoutBackgroundSound(library);

//print_info("CollectingPlaylist: collected '%s', new duration: %lld "
//	"(was %lld) frames\n", typeMarker.String(), Duration(), oldDuration);

	return ret;
}

// #pragma mark -

// SetTransitionClipID
void
CollectingPlaylist::SetTransitionClipID(const char* transitionClipID)
{
	SetValue(PROPERTY_CLIP_ID, transitionClipID);
}

// TransitionClipID
const char*
CollectingPlaylist::TransitionClipID() const
{
	return Value(PROPERTY_CLIP_ID, (const char*)NULL);
}

// SetSoundClipID
void
CollectingPlaylist::SetSoundClipID(const char* soundClipID)
{
	SetValue(PROPERTY_BACKGROUND_SOUND_ID, soundClipID);
}

// SoundClipID
const char*
CollectingPlaylist::SoundClipID() const
{
	return Value(PROPERTY_BACKGROUND_SOUND_ID, (const char*)NULL);
}

// SetTypeMarker
void
CollectingPlaylist::SetTypeMarker(const char* typeMarker)
{
	SetValue(PROPERTY_TYPE_MARKER, typeMarker);
}

// TypeMarker
const char*
CollectingPlaylist::TypeMarker() const
{
	return Value(PROPERTY_TYPE_MARKER, (const char*)NULL);
}

// SetItemDuration
void
CollectingPlaylist::SetItemDuration(uint64 duration)
{
	SetValue(PROPERTY_ITEM_DURATION, (int64)duration);
}

// ItemDuration
uint64
CollectingPlaylist::ItemDuration() const
{
	return Value(PROPERTY_ITEM_DURATION, (int64)(9 * 25));
}

// CollectorMode
int32
CollectingPlaylist::CollectorMode() const
{
	OptionProperty* mode = dynamic_cast<OptionProperty*>(
		FindProperty(PROPERTY_COLLECTOR_MODE));
	if (mode)
		return mode->Value();
	return COLLECTOR_MODE_SEQUENCE;
}

// #pragma mark -

// compare_collectables
static int
compare_collectables(const void* _a, const void* _b)
{
	CollectablePlaylist* a = (CollectablePlaylist*)*(void**)_a;
	CollectablePlaylist* b = (CollectablePlaylist*)*(void**)_b;

	// compare by start valid date
	time_t nowSeconds = time(NULL);

	time_t startSecondsA = parsedate(a->StartDate(), nowSeconds);
	time_t startSecondsB = parsedate(b->StartDate(), nowSeconds);

	if (startSecondsA > startSecondsB)
		return -1;
	else if (startSecondsA < startSecondsB)
		return 1;

	// compare by name
	BString nameA(a->Name());
	BString nameB(b->Name());

	int result = nameA.Compare(nameB);

	// compare by sequence index
	if (result == 0)
		result = max_c(-1, min_c(1, a->SequenceIndex() - b->SequenceIndex()));
	return result;
}

// _CollectSequence
status_t
CollectingPlaylist::_CollectSequence(BList& collectables,
	const ServerObjectManager* library)
{
	collectables.SortItems(compare_collectables);

	// find the transition clip, if we are supposed to have one
	Clip* transitionClip = NULL;
	BString transitionClipID = TransitionClipID();

	if (transitionClipID.Length() > 0) {
		transitionClip = dynamic_cast<Clip*>(library->FindObject(
			transitionClipID.String()));
		if (!transitionClip) {
			print_error("CollectingPlaylist::_CollectSequence() - "
				"didn't find transition clip: %s (ignoring)\n",
				transitionClipID.String());
		}
	}

	BString previousName;

	int64 startFrame = 0;
	uint64 maxDuration = Value(PROPERTY_DURATION, (int64)0);
	uint64 defaultDuration = ItemDuration();

	int32 count = collectables.CountItems();
	for (int32 i = 0; i < count; i++) {
		CollectablePlaylist* collectable
			= (CollectablePlaylist*)collectables.ItemAtFast(i);

		uint64 itemDuration = collectable->PreferredDuration();
		if (itemDuration == 0)
			itemDuration = defaultDuration;
		if (maxDuration > 0 && startFrame + itemDuration > maxDuration) {
			// we would go past our maximum duration, so stop here
			break;
		}

		ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(collectable);
			// the item, if it was created, has it's own reference now
		if (!item) {
			print_error("CollectingPlaylist::_CollectSequence() - "
				"no memory to create ClipPlaylistItem\n");
			return B_NO_MEMORY;
		}

		BString name(collectable->Name());
		if (i > 0 && name != previousName && transitionClip) {
//printf("new sequence starts at %ld\n", i);
			// a new sequence starts
			ClipPlaylistItem* transitionItem
				= new (nothrow) ClipPlaylistItem(transitionClip);
			if (!transitionItem || !AddItem(transitionItem)) {
				delete transitionItem;
				print_error("CollectingPlaylist::_CollectSequence() - "
					"no memory to create/add ClipPlaylistItem (transition)\n");
				return B_NO_MEMORY;
			}

			transitionItem->SetStartFrame(startFrame);
			int64 transitionDuration = transitionClip->Duration();
			transitionItem->SetDuration(transitionDuration);
			startFrame += transitionDuration;
		}

		if (!AddItem(item)) {
			delete item;
			print_error("CollectingPlaylist::_CollectSequence() - "
				"no memory to add ClipPlaylistItem\n");
			return B_NO_MEMORY;
		}

		item->SetStartFrame(startFrame);
		item->SetDuration(itemDuration);
		item->SetTrack(0);

		startFrame += itemDuration;
		previousName = name;
	}
	return B_OK;
}

// _CollectRandom
status_t
CollectingPlaylist::_CollectRandom(BList& collectables,
	const ServerObjectManager* library)
{
	uint64 defaultDuration = ItemDuration();

	int32 count = collectables.CountItems();
	int32 index = rand() % count;

	CollectablePlaylist* collectable
		= (CollectablePlaylist*)collectables.ItemAtFast(index);

	uint64 itemDuration = collectable->PreferredDuration();
	if (itemDuration == 0)
		itemDuration = defaultDuration;

	ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(collectable);
		// the item, if it was created, has it's own reference now
	if (!item) {
		print_error("CollectingPlaylist::_CollectRandom() - "
			"no memory to create ClipPlaylistItem\n");
		return B_NO_MEMORY;
	}

	if (!AddItem(item)) {
		delete item;
		print_error("CollectingPlaylist::_CollectRandom() - "
			"no memory to add ClipPlaylistItem\n");
		return B_NO_MEMORY;
	}

	item->SetStartFrame(0);
	item->SetDuration(itemDuration);
	item->SetTrack(0);

	return B_OK;
}

// _LayoutBackgroundSound
status_t
CollectingPlaylist::_LayoutBackgroundSound(const ServerObjectManager* library)
{
	// find the background sound clip, if we are supposed to have one
	Clip* soundClip = NULL;
	BString soundClipID = SoundClipID();

	if (soundClipID.Length() > 0) {
		soundClip = dynamic_cast<Clip*>(library->FindObject(
			soundClipID.String()));
		if (!soundClip) {
			print_error("CollectingPlaylist::_LayoutBackgroundSound() - "
				"didn't background sound clip: %s (ignoring)\n",
				soundClipID.String());
			return B_OK;
		}
	} else {
		// no background sound configured
		return B_OK;
	}

	float volume = Value(PROPERTY_BACKGROUND_SOUND_VOLUME, (float)1.0);

	uint64 duration = Duration();
	uint64 startFrame = 0;
	while (startFrame < duration) {
		ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(soundClip);
		if (!item) {
			print_error("CollectingPlaylist::_LayoutBackgroundSound() - "
				"no memory to create ClipPlaylistItem\n");
			return B_NO_MEMORY;
		}
	
		uint64 itemDuration = soundClip->Duration();
		uint64 maxItemDuration = duration - startFrame;

		if (startFrame == 0 && itemDuration >= maxItemDuration) {
			// one item as long as first track or longer
			// cut off
			itemDuration = maxItemDuration;
			// fade in + fade out
			PropertyAnimator* animator = item->AlphaAnimator();
			if (animator) {
				// remove all keyframes to get a clean start
				animator->MakeEmpty();
				KeyFrame* first = animator->InsertKeyFrameAt(0LL);
				KeyFrame* fadeEnd = animator->InsertKeyFrameAt(3);
				KeyFrame* fadeStart = animator->InsertKeyFrameAt(
					itemDuration - 4);
				KeyFrame* last = animator->InsertKeyFrameAt(itemDuration - 1);

				if (!first || !fadeEnd || !fadeStart || !last) {
					delete item;
					print_error("CollectingPlaylist::_LayoutBackgroundSound()"
						" - no memory to add fade keyframes\n");
					return B_NO_MEMORY;
				}

				first->SetScale(0.0);
				fadeEnd->SetScale(volume);
				fadeStart->SetScale(volume);
				last->SetScale(0.0);
			}
		} else if (startFrame == 0) {
			// first item, more to come
			// fade in
			PropertyAnimator* animator = item->AlphaAnimator();
			if (animator) {
				// remove all keyframes to get a clean start
				animator->MakeEmpty();
				KeyFrame* first = animator->InsertKeyFrameAt(0LL);
				KeyFrame* fadeEnd = animator->InsertKeyFrameAt(3);
				KeyFrame* last = animator->InsertKeyFrameAt(itemDuration - 1);

				if (!first || !fadeEnd || !last) {
					delete item;
					print_error("CollectingPlaylist::_LayoutBackgroundSound()"
						" - no memory to add fade keyframes\n");
					return B_NO_MEMORY;
				}

				first->SetScale(0.0);
				fadeEnd->SetScale(volume);
				last->SetScale(volume);
			}
		} else if (itemDuration >= maxItemDuration) {
			// last item
			// cut off
			itemDuration = maxItemDuration;
			// fade out
			PropertyAnimator* animator = item->AlphaAnimator();
			if (animator) {
				// remove all keyframes to get a clean start
				animator->MakeEmpty();
				KeyFrame* first = animator->InsertKeyFrameAt(0LL);
				KeyFrame* fadeStart = animator->InsertKeyFrameAt(
					itemDuration - 4);
				KeyFrame* last = animator->InsertKeyFrameAt(itemDuration - 1);

				if (!first || !fadeStart || !last) {
					delete item;
					print_error("CollectingPlaylist::_LayoutBackgroundSound()"
						" - no memory to add fade keyframes\n");
					return B_NO_MEMORY;
				}

				first->SetScale(volume);
				fadeStart->SetScale(volume);
				last->SetScale(0.0);
			}
		} else {
			// any remaining item
			PropertyAnimator* animator = item->AlphaAnimator();
			if (animator) {
				// remove all keyframes to get a clean start
				animator->MakeEmpty();
				KeyFrame* first = animator->InsertKeyFrameAt(0LL);

				if (!first) {
					delete item;
					print_error("CollectingPlaylist::_LayoutBackgroundSound()"
						" - no memory to add fade keyframes\n");
					return B_NO_MEMORY;
				}

				first->SetScale(volume);
			}
		}

		item->SetStartFrame(startFrame);
		item->SetDuration(itemDuration);
		item->SetTrack(1);

		if (!AddItem(item)) {
			delete item;
			print_error("CollectingPlaylist::_LayoutBackgroundSound() - "
				"no memory to add ClipPlaylistItem\n");
			return B_NO_MEMORY;
		}

		startFrame += itemDuration;
	}

	return B_OK;
}



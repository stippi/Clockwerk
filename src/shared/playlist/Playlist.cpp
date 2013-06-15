/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Playlist.h"

#include <new>

#include <stdio.h>

#include <Archivable.h>

#include "common.h"
#include "support_date.h"

#include "CommonPropertyIDs.h"
#include "DurationProperty.h"
#include "Icons.h"
#include "Painter.h"
#include "PlaylistItem.h"
#include "PlaylistObserver.h"
#include "PropertyObjectFactory.h"
#include "ServerObjectManager.h"
#include "TrackProperties.h"

using std::nothrow;

// constructor
Playlist::Playlist()
	:
	Clip("Playlist"),
	fItems(64),
	fTrackProperties(16),
	fSoloTrack(-1),
	fObservers(4),
	fNotificationBlocks(0),

	fDuration(0),
	fMaxTrack(0),
	fChangeToken(0),

	fDurationProperty(dynamic_cast<DurationProperty*>(
		FindProperty(PROPERTY_DURATION_INFO)))
{
}

// constructor
Playlist::Playlist(const char* type)
	:
	Clip(type),
	fItems(64),
	fTrackProperties(16),
	fSoloTrack(-1),
	fObservers(4),
	fNotificationBlocks(0),

	fDuration(0),
	fMaxTrack(0),
	fChangeToken(0),

	fDurationProperty(dynamic_cast<DurationProperty*>(
		FindProperty(PROPERTY_DURATION_INFO)))
{
}

// constructor
Playlist::Playlist(const Playlist& other, bool deep)
	:
	Clip(other, deep),
	fItems(64),
	fTrackProperties(16),
	fSoloTrack(other.fSoloTrack),
	fObservers(4),
	fNotificationBlocks(0),

	fDuration(0),
	fMaxTrack(0),
	fChangeToken(0),

	fDurationProperty(dynamic_cast<DurationProperty*>(
		FindProperty(PROPERTY_DURATION_INFO)))
{
	SetTo(&other);
}

// destructor
Playlist::~Playlist()
{
	// delete the contained items
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++)
		delete ItemAtFast(i);

	// delete track properties
	count = CountTrackProperties();
	for (int32 i = 0; i < count; i++)
		delete TrackPropertiesAtFast(i);

	// debugging...
	int32 observerCount = fObservers.CountItems();
	if (observerCount > 0) {
		char message[256];
		sprintf(message, "Playlist::~Playlist() - "
				"there are still %ld observers watching!\n", observerCount);
		debugger(message);
	}
}

// SelectedChanged
void
Playlist::SelectedChanged()
{
	Notify();
}

// #pragma mark -

// SetTo
status_t
Playlist::SetTo(const ServerObject* _other)
{
	const Playlist* other = dynamic_cast<const Playlist*>(_other);
	if (!other)
		return B_ERROR;

	MakeEmpty();

	StartNotificationBlock();

	// clone all the contained items and put the
	// clones in this container's list
	int32 count = other->CountItems();
	bool error = false;
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* clone = other->ItemAtFast(i)->Clone(true);
		if (clone) {
			if (!AddItem(clone)) {
				// no memory to put the clone in the list
				print_error("Playlist::SetTo() -  no mem for AddItem()!\n");
				delete clone;
				error = true;
				break;
			}
		} else {
			// no memory to clone the item
			print_error("Playlist::SetTo() - no mem for clone!\n");
			error = true;
			break;
		}
	}
	count = other->CountTrackProperties();
	for (int32 i = 0; i < count; i++) {
		TrackProperties* clone = new (nothrow) TrackProperties(
			*other->TrackPropertiesAtFast(i));
		if (clone) {
			if (!AddTrackProperties(clone)) {
				// no memory to put the clone in the list
				print_error("Playlist::SetTo() -  no mem for AddItem()!\n");
				delete clone;
				error = true;
				break;
			}
		} else {
			// no memory to clone the item
			print_error("Playlist::SetTo() - no mem for clone!\n");
			error = true;
			break;
		}
	}

	_SetMaxTrack(other->fMaxTrack);
	fSoloTrack = other->fSoloTrack;

	FinishNotificationBlock();
	if (CountItems() > 0)
		_ItemsChanged();

	// we updated the duration in _ItemsChanged()
	// but in order to be a true clone, we adjust
	// back to the potentially overridden duration
	// of the original playlist
	_SetDuration(other->fDuration);

	// NOTE: PlaylistObservers are not copied on purpose
	if (error)
		return B_NO_MEMORY;

	return Clip::SetTo(_other);
}

// IsMetaDataOnly
bool
Playlist::IsMetaDataOnly() const
{
	return false;
}

// ResolveDependencies
status_t
Playlist::ResolveDependencies(const ServerObjectManager* library)
{
	SetDependenciesResolved(true);
		// temporarly prevent any cyclic dependencies resulting
		// in infinite loops

	status_t ret = B_OK;
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		if (ItemAtFast(i)->ResolveDependencies(library) < B_OK)
			ret = B_ERROR;
	}
	return ret;
}

// #pragma mark -

//// XMLStore
//status_t
//Playlist::XMLStore(XMLHelper& xml) const
//{
//}
//
//status_t
//Playlist::XMLRestore(XMLHelper& xml)
//{
//}

// Archive
status_t
Playlist::Archive(BMessage* into, bool deep) const
{
	if (into == NULL)
		return B_BAD_VALUE;

	status_t ret = B_OK;

	// store all track properties
	{
		int32 trackPropertyCount = fTrackProperties.CountItems();
		BMessage trackProperties;
		for (int32 i = 0; i < trackPropertyCount; i++) {
			TrackProperties* properties = reinterpret_cast<TrackProperties*>(
				fTrackProperties.ItemAtFast(i));
			BMessage propertiesArchive;
			ret = properties->Archive(&propertiesArchive);
			if (ret == B_OK) {
				ret = trackProperties.AddMessage("track properties",
					&propertiesArchive);
			}
			if (ret != B_OK)
				return ret;
		}
		ret = into->AddMessage("tracks", &trackProperties);
		if (ret != B_OK)
			return ret;
	}

	// store items
	{
		int32 itemCount = fItems.CountItems();
		BMessage items;
		for (int32 i = 0; i < itemCount; i++) {
			PlaylistItem* item = reinterpret_cast<PlaylistItem*>(
				fItems.ItemAtFast(i));
			BMessage itemArchive;
			ret = item->Archive(&itemArchive);
			if (ret == B_OK)
				ret = items.AddMessage("item", &itemArchive);
			if (ret != B_OK)
				return ret;
		}
		ret = into->AddMessage("items", &items);
		if (ret != B_OK)
			return ret;
	}

	// store the rest of the properties
	if (ret == B_OK && fSoloTrack >= 0)
		ret = into->AddInt32("solo track", fSoloTrack);

	return ret;
}

// Unarchive
status_t
Playlist::Unarchive(const BMessage* from)
{
	if (from == NULL)
		return B_BAD_VALUE;

	MakeEmpty();

	status_t ret = B_OK;

	// restore all track properties
	{
		BMessage trackProperties;
		ret = from->FindMessage("tracks", &trackProperties);
		if (ret != B_OK) {
			fprintf(stderr, "Playlist::Unarchive() - "
				"Unable to find track properties archive!");
			return ret;
		}

		BMessage propertiesArchive;
		for (int32 i = 0; trackProperties.FindMessage("track properties", i,
				&propertiesArchive) == B_OK; i++) {
			TrackProperties properties(&propertiesArchive);
			if (!SetTrackProperties(properties))
				return B_NO_MEMORY;
		}
	}

	// restore items
	{
		BMessage items;
		ret = from->FindMessage("items", &items);
		if (ret != B_OK) {
			fprintf(stderr, "Playlist::Unarchive() - "
				"Unable to find items archive!");
			return ret;
		}
		BMessage itemArchive;
		for (int32 i = 0; items.FindMessage("item", i,
				&itemArchive) == B_OK; i++) {
			BArchivable* archivable = instantiate_object(&itemArchive);
			PlaylistItem* item = dynamic_cast<PlaylistItem*>(archivable);
			if (item == NULL) {
				// Maybe a newer Playlist archive format?
				delete archivable;
			} else if (!AddItem(item)) {
				delete item;
				return B_NO_MEMORY;
			}
		}
	}

	// store the rest of the properties
	int32 soloTrack;
	if (from->FindInt32("solo track", &soloTrack) == B_OK)
		fSoloTrack = soloTrack;

	ValidateItemLayout();

	return B_OK;
}

// #pragma mark -

// Duration
uint64
Playlist::Duration()
{
	return fDuration;
}

// MaxDuration
uint64
Playlist::MaxDuration()
{
	ValidateItemLayout();
	return Duration();
}

// Bounds
BRect
Playlist::Bounds(BRect canvasBounds)
{
	return canvasBounds;
}

// GetIcon
bool
Playlist::GetIcon(BBitmap* icon)
{
	return GetBuiltInIcon(icon, kPlaylistIcon);
}

// #pragma mark -

// Clone
Playlist*
Playlist::Clone(bool deep) const
{
	return new (nothrow) Playlist(*this, deep);
}

// SetCurrentFrame
void
Playlist::SetCurrentFrame(double frame)
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		if (item->StartFrame() <= frame
			&& item->EndFrame() >= floor(frame))
			item->SetCurrentFrame(frame);
	}
}

// Width
int32
Playlist::Width() const
{
	return Value(PROPERTY_WIDTH, (int32)640);
}

// Height
int32
Playlist::Height() const
{
	return Value(PROPERTY_HEIGHT, (int32)480);
}

// MouseDown
bool
Playlist::MouseDown(BPoint where, uint32 buttons, BRect canvasBounds,
	double frame, PlaybackNavigator* navigator)
{
//printf("%p->Playlist::MouseDown(BPoint(%.1f, %.1f), %ld, "
//	"BRect(%.1f, %.1f, %.1f, %.1f), %p)\n", this, where.x, where.y,
//	buttons, canvasBounds.left, canvasBounds.top, canvasBounds.right,
//	canvasBounds.bottom, navigator);
	// NOTE: where is already in local coordinate space

	BRect bounds = Bounds(canvasBounds);
	if (!bounds.Contains(where))
		return false;

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);

		// ignore items that are not at the current frame
		if (item->StartFrame() > frame
			|| item->EndFrame() < floor(frame))
			continue;

		// ignore items who's track is disabled
		if (!IsTrackEnabled(item->Track()))
			continue;

		AffineTransform transform = item->Transformation();
		BPoint localWhere = transform.InverseTransform(where);
		double localFrame = frame - item->StartFrame();
		if (item->Bounds(bounds, false).Contains(localWhere)
			&& item->MouseDown(localWhere, buttons, bounds, localFrame,
				navigator)) {
			return true;
		}
	}

	return false;
}

// #pragma mark -

// ValidateItemLayout
void
Playlist::ValidateItemLayout()
{
	// we only make sure that our duration is correct
	_ItemsChanged();
}

// #pragma mark -

// AddListObserver
void
Playlist::AddListObserver(PlaylistObserver* observer)
{
	if (observer && !fObservers.HasItem((void*)observer)) {
		if (!fObservers.AddItem((void*)observer)) {
			print_error("Playlist::AddListObserver) - "
				"no memory to add observer!");
		}
	}
}

// RemoveListObserver
void
Playlist::RemoveListObserver(PlaylistObserver* observer)
{
	fObservers.RemoveItem((void*)observer);
}

// StartNotificationBlock
void
Playlist::StartNotificationBlock()
{
	fNotificationBlocks++;
	if (fNotificationBlocks == 1)
		_NotifyNotificationBlockStarted();
}

// FinishNotificationBlock
void
Playlist::FinishNotificationBlock()
{
	fNotificationBlocks--;
	if (fNotificationBlocks == 0)
		_NotifyNotificationBlockFinished();
}

// #pragma mark -

// AddItem
bool
Playlist::AddItem(PlaylistItem* item)
{
	return AddItem(item, CountItems());
}

// AddItem
bool
Playlist::AddItem(PlaylistItem* item, int32 index)
{
	AutoNotificationSuspender _(this);

	if (item && fItems.AddItem((void*)item, index)) {
		item->SetParent(this);
		_ItemsChanged();
		_NotifyItemAdded(item, index);
		return true;
	}
	return false;
}

// RemoveItem
PlaylistItem*
Playlist::RemoveItem(int32 index)
{
	AutoNotificationSuspender _(this);

	PlaylistItem* item = (PlaylistItem*)fItems.RemoveItem(index);
	if (item) {
		item->SetParent(NULL);
		_ItemsChanged();
		_NotifyItemRemoved(item);
	}
	return item;
}

// RemoveItem
bool
Playlist::RemoveItem(PlaylistItem* item)
{
	AutoNotificationSuspender _(this);

	if (fItems.RemoveItem((void*)item)) {
		item->SetParent(NULL);
		_ItemsChanged();
		_NotifyItemRemoved(item);
		return true;
	}
	return false;
}

// ItemAt
PlaylistItem*
Playlist::ItemAt(int32 index) const
{
	return (PlaylistItem*)fItems.ItemAt(index);
}

// ItemAtFast
PlaylistItem*
Playlist::ItemAtFast(int32 index) const
{
	return (PlaylistItem*)fItems.ItemAtFast(index);
}

// HasItem
bool
Playlist::HasItem(PlaylistItem* item) const
{
	return fItems.HasItem((void*)item);
}

// IndexOf
int32
Playlist::IndexOf(PlaylistItem* item) const
{
	return fItems.IndexOf((void*)item);
}

// CountItems
int32
Playlist::CountItems() const
{
	return fItems.CountItems();
}

// MakeEmpty
void
Playlist::MakeEmpty()
{
	// delete items
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		PlaylistItem* item = ItemAtFast(i);
		_NotifyItemRemoved(item);
		delete item;
	}
	fItems.MakeEmpty();

	// delete track properties
	count = CountTrackProperties();
	for (int32 i = 0; i < count; i++) {
		delete TrackPropertiesAtFast(i);
		_NotifyTrackPropertiesChanged(i);
	}
	fTrackProperties.MakeEmpty();
}

// SortItems
void
Playlist::SortItems(int (*cmp)(const void*, const void*))
{
	fItems.SortItems(cmp);
}

// #pragma mark -

// SetTrackProperties
bool
Playlist::SetTrackProperties(const TrackProperties& properties)
{
	bool exists;
	int32 index = _IndexForTrackProperties(properties.Track(), &exists);
	if (exists) {
		TrackProperties* existing = TrackPropertiesAtFast(index);
		if (*existing != properties) {
			*existing = properties;
			_NotifyTrackPropertiesChanged(properties.Track());
		}
		return true;
	} else {
		if (fTrackProperties.AddItem(
				new TrackProperties(properties), index)) {
			_NotifyTrackPropertiesChanged(properties.Track());
			return true;
		}
		return false;
	}
}

// ClearTrackProperties
bool
Playlist::ClearTrackProperties(uint32 track)
{
	int32 count = CountTrackProperties();
	for (int32 i = 0; i < count; i++) {
		TrackProperties* properties = TrackPropertiesAtFast(i);
		if (properties->Track() == track) {
			if (fTrackProperties.RemoveItem(properties)) {
				_NotifyTrackPropertiesChanged(properties->Track());
				delete properties;
				return true;
			}
			break;
		}
	}
	return false;
}

// PropertiesForTrack
TrackProperties*
Playlist::PropertiesForTrack(uint32 track) const
{
	bool exists;
	int32 index = _IndexForTrackProperties(track, &exists);
	if (exists)
		return TrackPropertiesAtFast(index);
	return NULL;
}

// TrackPropertiesAt
TrackProperties*
Playlist::TrackPropertiesAt(int32 index) const
{
	return (TrackProperties*)fTrackProperties.ItemAt(index);
}

// TrackPropertiesAtFast
TrackProperties*
Playlist::TrackPropertiesAtFast(int32 index) const
{
	return (TrackProperties*)fTrackProperties.ItemAtFast(index);
}

// CountTrackProperties
int32
Playlist::CountTrackProperties() const
{
	return fTrackProperties.CountItems();
}

// SetSoloTrack
void
Playlist::SetSoloTrack(int32 track)
{
	if (fSoloTrack == track)
		return;

	if (fSoloTrack >= 0)
		_NotifyTrackPropertiesChanged(fSoloTrack);

	fSoloTrack = track;

	if (fSoloTrack >= 0)
		_NotifyTrackPropertiesChanged(fSoloTrack);
}

// IsTrackEnabled
bool
Playlist::IsTrackEnabled(uint32 track) const
{
	if (fSoloTrack >= 0)
		return track == (uint32)fSoloTrack;

	if (TrackProperties* properties = PropertiesForTrack(track))
		return properties->IsEnabled();

	return true;
}

// MoveTrack
void
Playlist::MoveTrack(uint32 oldIndex, uint32 newIndex)
{
	if (oldIndex == newIndex)
		return;

	StartNotificationBlock();

	// the range of effected tracks
	uint32 minTrack = min_c(oldIndex, newIndex);
	uint32 maxTrack = max_c(oldIndex, newIndex);

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		uint32 track = item->Track();
		if (track >= minTrack && track <= maxTrack) {
			// item is affected
			if (track == oldIndex) {
				// item is on track being moved
				item->SetTrack(newIndex);
			} else if (track > oldIndex) {
				// item is below track being moved,
				// means track moves down, so item moves up
				item->SetTrack(track - 1);
			} else {
				// item is above track being moved,
				// means track moves up, so item moves down
				item->SetTrack(track + 1);
			}
		}
	}

	TrackProperties* movingProperties = NULL;

	// rearrange track properties
	count = CountTrackProperties();
	for (int32 i = 0; i < count; i++) {
		TrackProperties* properties = TrackPropertiesAtFast(i);
		uint32 track = properties->Track();
		if (track >= minTrack && track <= maxTrack) {
			// track properties are affected
			if (track == oldIndex) {
				// item is on track being moved
				properties->SetTrack(newIndex);
				movingProperties = properties;
			} else if (track > oldIndex) {
				// item is below track being moved,
				// means track moves down, so item moves up
				properties->SetTrack(track - 1);
			} else {
				// item is above track being moved,
				// means track moves up, so item moves down
				properties->SetTrack(track + 1);
			}
		}
		if (track > maxTrack)
			break;
	}
	if (movingProperties) {
		// take care of keeping the track properties list sorted
		fTrackProperties.RemoveItem(movingProperties);
		bool exists;
		int32 insertIndex = _IndexForTrackProperties(newIndex, &exists);
			// exists cannot be "true", since we moved all properties
			// including the one at the target track!
		fTrackProperties.AddItem(movingProperties, insertIndex);
	}

	_NotifyTrackMoved(oldIndex, newIndex);

	FinishNotificationBlock();
}

// InsertTrack
void
Playlist::InsertTrack(uint32 index)
{
	StartNotificationBlock();

	// push items and track properties on and below index one track down

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		uint32 track = item->Track();
		if (track >= index)
			item->SetTrack(track + 1);
	}

	count = CountTrackProperties();
	for (int32 i = 0; i < count; i++) {
		// NOTE: could be optimized because track properties
		// list is sorted by track...
		TrackProperties* properties = TrackPropertiesAtFast(i);
		uint32 track = properties->Track();
		if (track >= index)
			properties->SetTrack(track + 1);
	}

	_NotifyTrackInserted(index);

	FinishNotificationBlock();
}

// RemoveTrack
void
Playlist::RemoveTrack(uint32 index)
{
	StartNotificationBlock();

	// push items and track properties below index one track up
	// NOTE: the track is supposed to be empty already!

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		uint32 track = item->Track();
		if (track > index)
			item->SetTrack(track - 1);
	}

	count = CountTrackProperties();
	for (int32 i = 0; i < count; i++) {
		// NOTE: could be optimized because track properties
		// list is sorted by track...
		TrackProperties* properties = TrackPropertiesAtFast(i);
		uint32 track = properties->Track();
		if (track == index) {
			fTrackProperties.RemoveItem(i);
			i--;
			count--;
		} else if (track > index)
			properties->SetTrack(track - 1);
	}

	_NotifyTrackRemoved(index);

	FinishNotificationBlock();
}

// #pragma mark -

// MakeRoom
void
Playlist::MakeRoom(int64 fromFrame, int64 toFrame, uint32 track,
				   PlaylistItem* ignoreItem,
				   int64* effectedRangeStart, int64* pushedFrames)
{
	// push items arround to make room
	*effectedRangeStart = fromFrame;
	*pushedFrames = 0;

	int32 count = CountItems();
	// first pass: find the effected item that
	// has the smallest start frame (so that the list
	// doesn't need to be sorted)
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		// see if the item is on the right track and
		// is not the item being ignored
		if (item != ignoreItem && item->Track() == track) {
			int64 itemStart = item->StartFrame();
			int64 itemEnd = item->EndFrame();
			// see if the item intersects the range
			if (itemEnd >= fromFrame && itemStart <= toFrame) {
				// the item needs to be pushed, adjust range
				// the range might have to start earlier
				fromFrame = min_c(itemStart, fromFrame);
				// figure out how many frame to push
				int64 pushStart;
				if (*pushedFrames == 0) {
					// no other items where found
					// yet, we can take the item start
					// as basis for how much to push
					pushStart = itemStart;
				} else {
					pushStart = fromFrame;
				}

				*pushedFrames = max_c(toFrame - pushStart,
									  *pushedFrames);
			}
		}
	}

	if (*pushedFrames == 0)
		// there was enough room
		return;

	// second pass, push all effected items
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		// see if the item is on the right track and
		// is not the item being ignored
		if (item != ignoreItem && item->Track() == track) {
			int64 itemStart = item->StartFrame();
			int64 itemEnd = itemStart + item->Duration();
			// see if this item intersects the effected
			// range *OR* is behind the start of the range
			if (itemStart >= fromFrame) {
				// the item needs to be pushed
				itemStart += *pushedFrames;
				item->SetStartFrame(itemStart);
				toFrame = itemEnd + *pushedFrames;
			}
		}
	}
	// fromFrame was adjusted to the start frame of the first
	// effected item, so that's where the effected range started
	*effectedRangeStart = fromFrame;
}

// MoveItems
void
Playlist::MoveItems(int64 startFrame, int64 frames, uint32 track,
					PlaylistItem* ignoreItem)
{
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		// if the item is on the same track as our
		// item and is behind or on fPushedBackStart,
		// we need to undo the change
		if (item != ignoreItem && item->Track() == track) {
			int64 itemStart = item->StartFrame();
			if (itemStart >= startFrame) {
				// the item is affected
				itemStart += frames;
				item->SetStartFrame(itemStart);
			}
		}
	}
}

// MaxTrack
uint32
Playlist::MaxTrack() const
{
	uint32 track = 0;
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		track = max_c(track, ItemAtFast(i)->Track());
	}
	return track;
}

// ItemsChanged
void
Playlist::ItemsChanged()
{
	_ItemsChanged();
}

// GetFrameBounds
void
Playlist::GetFrameBounds(int64* firstFrame, int64* lastFrame) const
{
	*firstFrame = 0;
	*lastFrame = 0;

	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		*firstFrame = min_c(*firstFrame, ItemAtFast(i)->StartFrame());
		*lastFrame = max_c(*lastFrame, ItemAtFast(i)->EndFrame());
	}
}

// PrintToStream
void
Playlist::PrintToStream() const
{
	print_info("--------------Playlist '%s'--------------\n",
		Name().String());
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = ItemAtFast(i);
		print_info("%s [%0*lld] - %s [%0*lld]  '%s'\n",
			string_for_frame(item->StartFrame()).String(),
			5, item->StartFrame(),
			string_for_frame(item->EndFrame()).String(),
			5, item->EndFrame(),
			item->Name().String());
	}
	print_info("---------------------------------------------\n");
}


// #pragma mark -

// AddTrackProperties
bool
Playlist::AddTrackProperties(TrackProperties* properties)
{
	return fTrackProperties.AddItem(properties);
}

// _ItemsChanged
void
Playlist::_ItemsChanged()
{
	// find the smallest start time and
	// largest end time of any item that
	// we contain
	int32 count = CountItems();
	if (count > 0) {
		int64 startFrame = 9223372036854775807LL;
		int64 endFrame = -9223372036854775807LL;
		uint32 maxTrack = 0;
		for (int32 i = 0; i < count; i++) {
			PlaylistItem* item = ItemAtFast(i);

			int64 itemStartFrame = item->StartFrame();
			int64 itemEndFrame = item->EndFrame();
			uint32 track = item->Track();

			if (itemStartFrame < startFrame)
				startFrame = itemStartFrame;
			if (itemEndFrame > endFrame)
				endFrame = itemEndFrame;
			if (track > maxTrack)
				maxTrack = track;
		}
		uint64 newDuration = endFrame - startFrame + 1;
		_SetDuration(newDuration);
		_SetMaxTrack(maxTrack);
	} else {
		_SetDuration(0);
		_SetMaxTrack(0);
	}
	fChangeToken++;
}

// #pragma mark -

// disallowed constructor
Playlist::Playlist(const Playlist& other)
	: Clip(NULL)
	, fChangeToken(0)
	, fDurationProperty(NULL)
{
}

// _SetDuration
void
Playlist::_SetDuration(uint64 duration)
{
	if (fDuration != duration) {
		fDuration = duration;
		if (fDurationProperty)
			fDurationProperty->SetValue(duration);
		_NotifyDurationChanged(fDuration);
	}
}

// _SetMaxTrack
void
Playlist::_SetMaxTrack(uint32 maxTrack)
{
	if (fMaxTrack != maxTrack) {
		fMaxTrack = maxTrack;
		_NotifyMaxTrackChanged(fMaxTrack);
	}
}

// _IndexForTrackProperties
int32
Playlist::_IndexForTrackProperties(uint32 track, bool* exists) const
{
//printf("_IndexForTrackProperties(%ld)\n", track);
	// find the TrackProperties before or at track
	// if there are no Tracks earlier than track,
	// return 0

	*exists = false;
	// binary search
	int32 lower = 0;
	int32 upper = CountTrackProperties();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
//printf("  lower: %ld, upper: %ld -> mid: %ld\n", lower, upper, mid);
		uint32 midTrack = TrackPropertiesAtFast(mid)->Track();
		if (track < midTrack)
			upper = mid;
		else if (track == midTrack) {
			*exists = true;
			return mid;
		} else
			lower = mid + 1;
	}

	return lower;
}

// #pragma mark -

// _NotifyItemAdded
void
Playlist::_NotifyItemAdded(PlaylistItem* item, int32 index)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer =
			(PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->ItemAdded(this, item, index);
	}
}

// _NotifyItemRemoved
void
Playlist::_NotifyItemRemoved(PlaylistItem* item)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer =
			(PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->ItemRemoved(this, item);
	}
}

// _NotifyTrackPropertiesChanged
void
Playlist::_NotifyTrackPropertiesChanged(uint32 track)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer =
			(PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->TrackPropertiesChanged(this, track);
	}
}

// _NotifyTrackMoved
void
Playlist::_NotifyTrackMoved(uint32 oldIndex, uint32 newIndex)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer =
			(PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->TrackMoved(this, oldIndex, newIndex);
	}
}

// _NotifyTrackInserted
void
Playlist::_NotifyTrackInserted(uint32 track)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer =
			(PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->TrackInserted(this, track);
	}
}

// _NotifyTrackRemoved
void
Playlist::_NotifyTrackRemoved(uint32 track)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer =
			(PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->TrackRemoved(this, track);
	}
}

// _NotifyDurationChanged
void
Playlist::_NotifyDurationChanged(uint64 duration)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer
			= (PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->DurationChanged(this, duration);
	}
}

// _NotifyMaxTrackChanged
void
Playlist::_NotifyMaxTrackChanged(uint32 maxTrack)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer
			= (PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->MaxTrackChanged(this, maxTrack);
	}
}

// _NotifyNotificationBlockStarted
void
Playlist::_NotifyNotificationBlockStarted()
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer
			= (PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->NotificationBlockStarted(this);
	}
}

// _NotifyNotificationBlockFinished
void
Playlist::_NotifyNotificationBlockFinished()
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistObserver* observer
			= (PlaylistObserver*)fObservers.ItemAtFast(i);
		observer->NotificationBlockFinished(this);
	}
}


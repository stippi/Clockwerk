/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Schedule.h"

#include <new>

#include <stdio.h>

#include "common.h"
//#include "xml_export.h"
//#include "xml_import.h"

#include "AutoDeleter.h"
#include "ClipPlaylistItem.h"
#include "CommonPropertyIDs.h"
#include "HashSet.h"
#include "HashString.h"
#include "OptionProperty.h"
#include "Playlist.h"
#include "Property.h"
#include "ScheduleItem.h"
#include "ScheduleObserver.h"
#include "TimeProperty.h"
#include "WeekDaysProperty.h"
//#include "XMLHelper.h"

using std::nothrow;

// constructor
Schedule::Schedule()
	:
	ServerObject("Schedule"),
	fItems(64),
	fObservers(4),
	fNotificationBlocks(0),

	fType(dynamic_cast<OptionProperty*>(
		FindProperty(PROPERTY_SCHEDULE_TYPE))),
	fWeekDays(dynamic_cast<WeekDaysProperty*>(
		FindProperty(PROPERTY_WEEK_DAYS))),
	fDate(dynamic_cast<StringProperty*>(
		FindProperty(PROPERTY_DATE))),

	fLastTimeCheck(0),
	fCachedIndexAtCurrentFrame(-1)
{
}

// constructor
Schedule::Schedule(const char* type)
	:
	ServerObject(type),
	fItems(64),
	fObservers(4),
	fNotificationBlocks(0),

	fType(dynamic_cast<OptionProperty*>(
		FindProperty(PROPERTY_SCHEDULE_TYPE))),
	fWeekDays(dynamic_cast<WeekDaysProperty*>(
		FindProperty(PROPERTY_WEEK_DAYS))),
	fDate(dynamic_cast<StringProperty*>(
		FindProperty(PROPERTY_DATE))),

	fLastTimeCheck(0),
	fCachedIndexAtCurrentFrame(-1)
{
}

// constructor
Schedule::Schedule(const Schedule& other, bool deep)
	:
	ServerObject(other, deep),
	fItems(64),
	fObservers(4),
	fNotificationBlocks(0),

	fType(dynamic_cast<OptionProperty*>(
		FindProperty(PROPERTY_SCHEDULE_TYPE))),
	fWeekDays(dynamic_cast<WeekDaysProperty*>(
		FindProperty(PROPERTY_WEEK_DAYS))),
	fDate(dynamic_cast<StringProperty*>(
		FindProperty(PROPERTY_DATE))),

	fLastTimeCheck(0),
	fCachedIndexAtCurrentFrame(-1)
{
	SetTo(&other);
}

// destructor
Schedule::~Schedule()
{
	// delete the contained items
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++)
		ItemAtFast(i)->Release();

	// debugging...
	int32 observerCount = fObservers.CountItems();
	if (observerCount > 0) {
		char message[256];
		sprintf(message, "Schedule::~Schedule() - "
				"there are still %ld observers watching!\n", observerCount);
		debugger(message);
	}
}

// SelectedChanged
void
Schedule::SelectedChanged()
{
	Notify();
}

// #pragma mark -

// SetTo
status_t
Schedule::SetTo(const ServerObject* _other)
{
	const Schedule* other = dynamic_cast<const Schedule*>(_other);
	if (!other)
		return B_ERROR;

	MakeEmpty();

	ScheduleNotificationBlock _(this);

	// clone all the contained items and put the
	// clones in this container's list
	int32 count = other->CountItems();
	bool error = false;
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* clone = other->ItemAtFast(i)->Clone(true);
		if (clone) {
			if (!AddItem(clone)) {
				// no memory to put the clone in the list
				print_error("Schedule::SetTo() - no mem for AddItem()!\n");
				delete clone;
				error = true;
				break;
			}
		} else {
			// no memory to clone the item
			print_error("Schedule::SetTo() - no mem for clone!\n");
			error = true;
			break;
		}
	}
	// NOTE: ScheuduleObservers are not copied on purpose

	if (error)
		return B_NO_MEMORY;

	return ServerObject::SetTo(_other);
}

// IsMetaDataOnly
bool
Schedule::IsMetaDataOnly() const
{
	return false;
}

// ResolveDependencies
status_t
Schedule::ResolveDependencies(const ServerObjectManager* library)
{
	status_t ret = B_OK;
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		if (ItemAtFast(i)->ResolveDependencies(library) < B_OK)
			ret = B_ERROR;
	}
	return ret;
}

// #pragma mark -

#if 0
// XMLStore
status_t
Schedule::XMLStore(XMLHelper& xml) const
{
	status_t ret = xml.CreateTag("SCHEDULE");

	int32 count = CountItems();

	// store used playlists
	if (ret == B_OK)
		ret = xml.CreateTag("REFERENCED_OBJECTS");

	if (ret == B_OK) {
		// use a hash map to store referenced playlists only once
		HashSet<HashString> referencedPlaylists;

		for (int32 i = 0; i < count; i++) {
			BString playlistID;
			int32 version = -1;
			Playlist* playlist = ItemAtFast(i)->Playlist();
			if (playlist) {
				playlistID = playlist->ID();
				version = playlist->Version();
			} else {
				// schedule item might not be resolved properly,
				// try the referenced clip property instead
				playlistID = ItemAtFast(i)->Value(PROPERTY_CLIP_ID, "");
			}
			if (playlistID.Length() <= 0)
				continue;

			if (referencedPlaylists.Contains(playlistID.String()))
				continue;
			referencedPlaylists.Add(playlistID.String());

			ret = xml.CreateTag("OBJECT");
			if (ret < B_OK)
				break;

			ret = xml.SetAttribute("id", playlistID);
			if (ret < B_OK)
				break;

			if (version >= 0) {
				ret = xml.SetAttribute("version", version);
				if (ret < B_OK)
					break;
			}

			ret = xml.CloseTag(); // OBJECT
			if (ret < B_OK)
				break;
		}
	}

	if (ret == B_OK)
		ret = xml.CloseTag(); // REFERENCED_OBJECTS

	// store items
	if (ret == B_OK)
		ret = xml.CreateTag("ITEMS");

	if (ret == B_OK) {
		for (int32 i = 0; i < count; i++) {
			ret = xml.CreateTag("ITEM");
			if (ret < B_OK)
				break;

			ScheduleItem* item = ItemAtFast(i);

			Playlist* playlist = item->Playlist();
			if (playlist) {
				ret = xml.SetAttribute("playlist", playlist->ID());
				if (ret < B_OK)
					break;
			}

			ret = xml.SetAttribute("startframe", item->StartFrame());
			if (ret < B_OK)
				break;

			ret = xml.SetAttribute("frame_duration", item->Duration());
			if (ret < B_OK)
				break;

			if (!item->FlexibleStartFrame()) {
				ret = xml.SetAttribute("flexible_startframe",
					item->FlexibleStartFrame());
				if (ret < B_OK)
					break;
			}

			if (item->FlexibleDuration()) {
				ret = xml.SetAttribute("flexible_duration",
					item->FlexibleDuration());
				if (ret < B_OK)
					break;
			}

			if (item->ExplicitRepeats() > 0) {
				ret = xml.SetAttribute("repeats",
					(int32)item->ExplicitRepeats());
				if (ret < B_OK)
					break;
			}

			ret = xml.CloseTag(); // ITEM
			if (ret < B_OK)
				break;
		}
	}

	if (ret == B_OK)
		ret = xml.CloseTag(); // ITEMS

	if (ret == B_OK)
		ret = xml.CloseTag(); // SCHEDULE

	return ret;
}

//#define TRACE_SCHEDULE_PARSING
#ifdef TRACE_SCHEDULE_PARSING
# define TRACE_PARSE(x...)	printf(x)
#else
# define TRACE_PARSE(x...)
#endif

status_t
Schedule::XMLRestore(XMLHelper& xml)
{
	status_t ret = xml.OpenTag("SCHEDULE");
	if (ret < B_OK)
		return ret;

	TRACE_PARSE("opened SCHEDULE\n");

	ret = xml.OpenTag("ITEMS");
	if (ret < B_OK)
		return ret;

	TRACE_PARSE(" opened ITEMS\n");

	while (xml.OpenTag("ITEM") == B_OK) {
		BString playlistID = xml.GetAttribute("playlist", "");
		StringProperty* playlist = NULL;
		if (playlistID.CountChars() > 0) {
			playlist = new (nothrow) StringProperty(PROPERTY_CLIP_ID,
				playlistID.String());
		}

		TRACE_PARSE("  opened ITEM, playlist %s\n", playlistID.String());

		// retrieve the startFrame, duration and flexibleStartFrame
		// in a backwards compatible way (used to be saved in seconds,
		// so we distinguish by "starttime" versus "startframe")
		int64 duration;
		bool flexibleStartFrame;

		int64 startFrame = xml.GetAttribute("startframe", (int64)-1);
		if (startFrame >= 0) {
			duration = xml.GetAttribute("frame_duration", (int64)-1);
			flexibleStartFrame = xml.GetAttribute("flexible_startframe", true);
		} else {
			startFrame = (int64)xml.GetAttribute("starttime", (int32)-1) * 25;
			duration = (int64)xml.GetAttribute("duration", (int32)-1) * 25;
			flexibleStartFrame = xml.GetAttribute("flexible_starttime", true);
		}

		int32 explicitRepeats = -1;
		explicitRepeats = xml.GetAttribute("repeats", explicitRepeats);

		bool flexibleDuration = xml.GetAttribute("flexible_duration", false);

		TRACE_PARSE("   start:   %lld%s\n", startFrame,
			flexibleStartFrame ? " (flexible)" : "");
		TRACE_PARSE("   durtion: %lld%s\n", duration,
			flexibleDuration ? " (flexible)" : "");
		TRACE_PARSE("   repeats: %ld\n", explicitRepeats);

		if (startFrame >= 0 && duration > 0) {
			ScheduleItem* item = new (nothrow) ScheduleItem((::Playlist*)NULL);
			// try to add playlistID placeholder property to item
			if (item && playlist && !item->AddProperty(playlist)) {
				delete playlist;
				ret = B_NO_MEMORY;
			}
			// try to add item
			if (!item || !AddItem(item)) {
				delete item;
				ret = B_NO_MEMORY;
			}
			// if any of the above failed...
			if (ret < B_OK) {
				print_error("Schedule::XMLRestore() - "
					"no memory to create schedule item (schedule ID: %s)\n",
					ID().String());
				break;
			}
			item->SetStartFrame(startFrame);
			item->SetFlexibleDuration(flexibleDuration);
				// it is important to set this before setting
				// the duration, otherwise it gets filtered already
				// (allow == false for newly created items!)
			item->SetDuration(duration);
			item->SetFlexibleStartFrame(flexibleStartFrame);
			if (explicitRepeats > 0)
				item->SetExplicitRepeats((uint16)explicitRepeats);
		} else {
			print_error("Schedule::XMLRestore() - "
				"invalid schedule item in XML (schedule ID: %s)\n",
				ID().String());
		}

		ret = xml.CloseTag(); // ITEM
		if (ret < B_OK)
			break;
	}

	if (ret == B_OK)
		ret = xml.CloseTag(); // ITEMS

	TRACE_PARSE(" closed ITEMS\n");

	if (ret == B_OK)
		ret = xml.CloseTag(); // SCHEDULE

	TRACE_PARSE("closed SCHEDULE\n");

	SanitizeStartFrames();

	return ret;
}
#endif

// Archive
status_t
Schedule::Archive(BMessage* into, bool deep) const
{
	// TODO: Implement
	return B_ERROR;
}

// Unarchive
status_t
Schedule::Unarchive(const BMessage* from)
{
	// TODO: Implement
	return B_ERROR;
}

// #pragma mark -

// AddScheduleObserver
void
Schedule::AddScheduleObserver(ScheduleObserver* observer)
{
	if (observer && !fObservers.HasItem((void*)observer)) {
		if (!fObservers.AddItem((void*)observer)) {
			print_error("Schedule::AddScheduleObserver) - "
				"no memory to add observer!");
		}
	}
}

// RemoveScheduleObserver
void
Schedule::RemoveScheduleObserver(ScheduleObserver* observer)
{
	fObservers.RemoveItem((void*)observer);
}

// StartNotificationBlock
void
Schedule::StartNotificationBlock()
{
	fNotificationBlocks++;
	if (fNotificationBlocks == 1)
		_NotifyNotificationBlockStarted();
}

// FinishNotificationBlock
void
Schedule::FinishNotificationBlock()
{
	fNotificationBlocks--;
	if (fNotificationBlocks == 0)
		_NotifyNotificationBlockFinished();
}

// #pragma mark -

// Clone
Schedule*
Schedule::Clone(bool deep) const
{
	return new (nothrow) Schedule(*this, deep);
}

// GetCurrentPlaylist
void
Schedule::GetCurrentPlaylist(Playlist** playlist, uint64* startFrame) const
{
	ScheduleItem* item = ItemAt(IndexAtCurrentTime());
	if (item) {
		*playlist = item->Playlist();
		*startFrame = item->StartFrame();
	} else {
		*playlist = NULL;
		*startFrame = 0;
	}
}

// NextPlaylist
Playlist*
Schedule::NextPlaylist() const
{
	ScheduleItem* item = ItemAt(IndexAtCurrentTime() + 1);
	if (item)
		return item->Playlist();

	return NULL;
}

// IndexAtCurrentTime
int32
Schedule::IndexAtCurrentTime() const
{
// TODO: remove caching for the sake of a more precise playlist switch?
	bigtime_t now = system_time();
	if (fLastTimeCheck + 250000 < now) {
		fLastTimeCheck = now;

		// get current daytime
		time_t t = time(NULL);
		tm time = *localtime(&t);

		// calc the current second of the day since 0:00
		uint64 frame = (time.tm_hour * 60 * 60 + time.tm_min * 60
			+ time.tm_sec) * 25;

		int32 index = IndexAtFrame(frame);

if (fCachedIndexAtCurrentFrame != index) {
BString name = ItemAt(index) && ItemAt(index)->Playlist() ?
	ItemAt(index)->Playlist()->Name() : BString("NULL");
print_info("playlist for time %d:%d:%d = '%s' (index %ld [1...%ld])\n",
	time.tm_hour, time.tm_min, time.tm_sec, name.String(), index + 1,
	CountItems());
}

		fCachedIndexAtCurrentFrame = index;
	}
	return fCachedIndexAtCurrentFrame;
}

// #pragma mark -

// _CountFlexibleItemsInRange
int32
Schedule::_CountFlexibleItemsInRange(int32 index) const
{
	int32 count = 0;
	while (--index >= 0) {
		ScheduleItem* item = ItemAtFast(index);
		if (item->FlexibleDuration())
			count++;
		if (!item->FlexibleStartFrame())
			break;
	}
	return count;
}

// SanitizeStartFrames
void
Schedule::SanitizeStartFrames()
{
	ScheduleNotificationBlock _(this);

	uint64 startFrame = 0;
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* item = ItemAtFast(i);

		uint64 wantedStartFrame = startFrame;
		item->FilterStartFrame(&startFrame);

		item->SetStartFrame(startFrame);

		// make sure this item has the correct duration
		item->SetDuration(item->PreferredDuration());

		int64 diff = startFrame - wantedStartFrame;
			// if diff is positive, the item has placed itself
			// farther behind, and there is now a gap in the
			// schedule

		if (diff > 0) {
//printf("%ld, wanted: %llu -> startFrame: %llu, diff: %lld\n", i, wantedStartFrame, startFrame, diff);
			int32 flexibleCount = _CountFlexibleItemsInRange(i);
			int32 previousIndex = i;
			while (--previousIndex >= 0) {
				ScheduleItem* previous = ItemAtFast(previousIndex);
				if (previous->FlexibleDuration()) {
					// nice this item has a flexible duration or allows,
					// repeats we can simply add the diff, or at least
					// some part of it
					uint64 diffPortion;
					if (flexibleCount > 0)
						diffPortion = diff / flexibleCount;
					else
						diffPortion = diff;

					uint64 duration = previous->Duration() + diffPortion;
					previous->SetDuration(duration);

					diff -= diffPortion;
					flexibleCount--;
//printf("enlarging item %ld (%s) - diff: %lld\n",
//	previousIndex, previous->Playlist() ?
//		previous->Playlist()->Name().String() : NULL, diff);
				}
				if (previous->FlexibleStartFrame()) {
					// move the previous item
//printf("moving item %ld by %lld\n", previousIndex, diff);
					previous->SetStartFrame(previous->StartFrame() + diff);
				} else {
					// we have hit an item that has neither flexible duration
					// nor flexible startframe, so we have hit a dead-end
					// there will simply be a gap in the schedule now
					// TODO: worse yet, there could be overlapping items!
					// TODO: maybe a real layout (constraint solving)
					// algorithm is needed?
					break;
				}
			}
		}

		startFrame = item->StartFrame() + item->Duration();
	}

	_NotifyItemsLayouted();
}

// #pragma mark -

// GeneratePlaylist
Playlist*
Schedule::GeneratePlaylist(int64 _startFrame, int64 _endFrame,
	bool offsetToZero) const
{
	if (_endFrame < _startFrame)
		return NULL;

	Playlist* playlist = new (nothrow) Playlist();
	if (!playlist)
		return NULL;

	playlist->SetName("Schedule converted Playlist");

	ObjectDeleter<Playlist> playlistDeleter(playlist);

	int64 startFrameOffset = offsetToZero ? _startFrame : 0;
	int32 startIndex = _startFrame >= 0 ? IndexAtFrame(_startFrame) : 0;
	int32 endIndex = _endFrame >= 0 ? IndexAtFrame(_endFrame)
		: CountItems() - 1;

	for (int32 i = startIndex; i <= endIndex; i++) {
		ScheduleItem* scheduleItem = ItemAtFast(i);
		Playlist* itemPlaylist = scheduleItem->Playlist();

		int64 clipOffset = _startFrame - scheduleItem->StartFrame();

		int64 startFrame = scheduleItem->StartFrame();
		int64 endFrame = min_c(_endFrame, (int64)scheduleItem->StartFrame()
			+ (int64)scheduleItem->Duration() - 1);

		int64 itemDuration = endFrame - startFrame + 1;
		_startFrame += itemDuration - clipOffset;

		if (startFrame > kWholeDayDuration
			|| itemDuration > kWholeDayDuration) {
			// ignore invalid items
			continue;
		}
		if (itemPlaylist) {
			// repeat adding playlist items for as long as the playlist
			// is repeating in this schedule item
			while (itemDuration > 0) {
				ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(
					itemPlaylist);
				if (!item)
					return NULL;

				uint64 duration = min_c((uint64)itemDuration,
					itemPlaylist->Duration());
				if (duration == 0) {
					print_warning("skipping empty playlist\n");
					delete item;
					break;
				}

				item->SetStartFrame(startFrame - startFrameOffset);
				item->SetDuration(duration);
				if (clipOffset > 0) {
					item->SetClipOffset(clipOffset);
					clipOffset = 0;
				}
				if (!playlist->AddItem(item)) {
					delete item;
					return NULL;
				}
				itemDuration -= duration;
				startFrame += duration;
			}
		} else {
			ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(
				(Clip*)NULL);
			if (!item)
				return NULL;

			item->SetStartFrame(startFrame - startFrameOffset);
			item->SetDuration(itemDuration);
			if (clipOffset > 0) {
				item->SetClipOffset(clipOffset);
				clipOffset = 0;
			}

			if (!playlist->AddItem(item)) {
				delete item;
				return NULL;
			}
		}
	}

	playlistDeleter.Detach();
	return playlist;
}

// #pragma mark -

// InsertIndexAtFrame
int32
Schedule::InsertIndexAtFrame(uint64 frame) const
{
	// find the insertion index for a ScheduleItem at "frame"

	// binary search
	int32 lower = 0;
	int32 upper = CountItems();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
//printf("  lower: %ld, upper: %ld -> mid: %ld\n", lower, upper, mid);
		uint64 midFrame = ItemAtFast(mid)->StartFrame();
		if (frame <= midFrame)
			upper = mid;
		else
			lower = mid + 1;
	}

	return lower;
}

// IndexAtFrame
int32
Schedule::IndexAtFrame(uint64 frame) const
{
	// find the ScheduleItem before or at "frame"

	// * we add one frame, because the insertion index
	// would be one too low if it finds an item right
	// "at" the frame (it would insert before that item)
	// * we subtract one from the insertion index because
	// the index would be one too high (we want the item
	// "before" frame, and the insertion index would
	// point after that item)
	int32 index = InsertIndexAtFrame(frame + 1) - 1;
	ScheduleItem* item = ItemAt(index);
	if (item && item->StartFrame() + item->Duration() < frame)
		index = -1; // got item, but not long enough to cover time
	return index;
}

// AddItem
bool
Schedule::AddItem(ScheduleItem* item)
{
	return AddItem(item, CountItems());
}

// AddItem
bool
Schedule::AddItem(ScheduleItem* item, int32 index)
{
	AutoNotificationSuspender _(this);

	if (item && fItems.AddItem((void*)item, index)) {
		item->SetParent(this);
		_NotifyItemAdded(item, index);
		return true;
	}
	return false;
}

// RemoveItem
ScheduleItem*
Schedule::RemoveItem(int32 index)
{
	AutoNotificationSuspender _(this);

	ScheduleItem* item = (ScheduleItem*)fItems.RemoveItem(index);
	if (item) {
		item->SetParent(NULL);
		_NotifyItemRemoved(item);
	}
	return item;
}

// RemoveItem
bool
Schedule::RemoveItem(ScheduleItem* item)
{
	AutoNotificationSuspender _(this);

	if (fItems.RemoveItem((void*)item)) {
		item->SetParent(NULL);
		_NotifyItemRemoved(item);
		return true;
	}
	return false;
}

// ItemAt
ScheduleItem*
Schedule::ItemAt(int32 index) const
{
	return (ScheduleItem*)fItems.ItemAt(index);
}

// ItemAtFast
ScheduleItem*
Schedule::ItemAtFast(int32 index) const
{
	return (ScheduleItem*)fItems.ItemAtFast(index);
}

// HasItem
bool
Schedule::HasItem(ScheduleItem* item) const
{
	return fItems.HasItem((void*)item);
}

// IndexOf
int32
Schedule::IndexOf(ScheduleItem* item) const
{
	return fItems.IndexOf((void*)item);
}

// CountItems
int32
Schedule::CountItems() const
{
	return fItems.CountItems();
}

// MakeEmpty
void
Schedule::MakeEmpty()
{
	// release items
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		ScheduleItem* item = ItemAtFast(i);
		_NotifyItemRemoved(item);
		item->Release();
	}
	fItems.MakeEmpty();
}

// #pragma mark -

// #pragma mark -

// disallowed constructor
Schedule::Schedule(const Schedule& other)
	: ServerObject(NULL)
{
}

// _NotifyItemAdded
void
Schedule::_NotifyItemAdded(ScheduleItem* item, int32 index)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleObserver* observer =
			(ScheduleObserver*)fObservers.ItemAtFast(i);
		observer->ItemAdded(item, index);
	}
}

// _NotifyItemRemoved
void
Schedule::_NotifyItemRemoved(ScheduleItem* item)
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleObserver* observer =
			(ScheduleObserver*)fObservers.ItemAtFast(i);
		observer->ItemRemoved(item);
	}
}

// _NotifyItemsLayouted
void
Schedule::_NotifyItemsLayouted()
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleObserver* observer =
			(ScheduleObserver*)fObservers.ItemAtFast(i);
		observer->ItemsLayouted();
	}
}

// _NotifyNotificationBlockStarted
void
Schedule::_NotifyNotificationBlockStarted()
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleObserver* observer
			= (ScheduleObserver*)fObservers.ItemAtFast(i);
		observer->NotificationBlockStarted();
	}
}

// _NotifyNotificationBlockFinished
void
Schedule::_NotifyNotificationBlockFinished()
{
	int32 count = fObservers.CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleObserver* observer
			= (ScheduleObserver*)fObservers.ItemAtFast(i);
		observer->NotificationBlockFinished();
	}
}


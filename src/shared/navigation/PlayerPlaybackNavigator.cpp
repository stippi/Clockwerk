/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlayerPlaybackNavigator.h"

#include <new>

#include "common.h"

#include "ClipPlaylistItem.h"
#include "Event.h"
#include "EventQueue.h"
#include "NavigationInfo.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "RWLocker.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "ServerObjectManager.h"

using std::nothrow;


class PlayerPlaybackNavigator::NavigatorEvent : public Event {
public:
	NavigatorEvent(bigtime_t timeout, PlayerPlaybackNavigator* navigator)
		: Event(false)
		, fNavigator(navigator)
		, fTimeout(timeout)
		, fScheduled(false)
	{
	}

	virtual ~NavigatorEvent()
	{
	}

	virtual void Execute()
	{
		fScheduled = false;
		fNavigator->_NavigationTimeout();
	}

	void PushBack()
	{
		EventQueue::Default().ChangeEvent(this, system_time() + fTimeout);
	}

	void ScheduleSelf()
	{
		SetTime(system_time() + fTimeout);
		EventQueue::Default().AddEvent(this);
	}

	bool IsScheduled() const
	{
		return fScheduled;
	}

private:
	PlayerPlaybackNavigator*	fNavigator;
	bigtime_t					fTimeout;
	bool						fScheduled;
};


static const bigtime_t kDefaultTimeout = 30LL * 1000000;
static const uint32 kDummyItemTrack = 0;
static const uint32 kScheduleTrack = 1;
static const uint32 kNavigationTrack = 2;

// constructor
PlayerPlaybackNavigator::PlayerPlaybackNavigator(RWLocker* locker)
	: PlaybackNavigator()
	, fLocker(locker)
	, fObjectManager(NULL)
	, fSchedule(NULL)
	, fMasterPlaylist(new (nothrow) Playlist())
	, fPreviousPlaylist(NULL)
	, fCurrentPlaylist(NULL)
	, fCurrentScheduleIndex(-1)
	, fCurrentFrame(0.0)

	, fPreviousNavigationPlaylist(NULL)
	, fNavigationPlaylist(NULL)
	, fNavigationPlaylistStartFrame(-1000)

	, fNavigationEvent(new (nothrow) NavigatorEvent(kDefaultTimeout, this))
{
	if (!fMasterPlaylist || !fNavigationEvent) {
		print_error("no memory for master playlist or navigation event\n");
		fatal(B_NO_MEMORY);
	}

	fMasterPlaylist->SetName("master playlist");
	// add a dummy item which spans the whole day, so that the
	// playlist is never shorter
	ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(NULL, 0,
		kDummyItemTrack);
	if (item)
		item->SetDuration(24LL * 60 * 60 * 25);

	if (!item || !fMasterPlaylist->AddItem(item)) {
		print_error("no memory to create dummy whole day item\n");
		fatal(B_NO_MEMORY);
	}
}

// destructor
PlayerPlaybackNavigator::~PlayerPlaybackNavigator()
{
	fMasterPlaylist->Release();
}

// Navigate
void
PlayerPlaybackNavigator::Navigate(const NavigationInfo* info)
{
	if (!info || !fObjectManager)
		return;

	BString id(info->TargetID());
	fNavigationPlaylist = dynamic_cast<Playlist*>(
		fObjectManager->FindObject(id));

	if (fNavigationEvent->IsScheduled())
		fNavigationEvent->PushBack();
	else
		fNavigationEvent->ScheduleSelf();
}

// #pragma mark -

// SetObjectManager
void
PlayerPlaybackNavigator::SetObjectManager(ServerObjectManager* manager)
{
	fObjectManager = manager;
}

// SetSchedule
void
PlayerPlaybackNavigator::SetSchedule(Schedule* schedule)
{
	fSchedule = schedule;

	// remove all but the first playlist item (the dummy item that
	// forces the duration of the masterplaylist to span the whole day)
	fLocker->WriteLock();

	int32 count = fMasterPlaylist->CountItems();
	for (int32 i = count - 1; i >= 1; i--)
		delete fMasterPlaylist->RemoveItem(i);
	fCurrentScheduleIndex = -1;
	fCurrentPlaylist = NULL;

	fLocker->WriteUnlock();
}

//#define TRACE_SWITCH_PLAYLIST
#ifdef TRACE_SWITCH_PLAYLIST
# define TRACE_SP(x...) printf(x)
#else
# define TRACE_SP(x...)
#endif

// SetCurrentFrame
void
PlayerPlaybackNavigator::SetCurrentFrame(double _frame, bool& wasPlaying,
	bool& isPlaying)
{
	fCurrentFrame = _frame;
	int64 currentFrame = (int64)_frame;
	TRACE_SP("SetCurrentFrame(%lld)\n", currentFrame);

	wasPlaying = fPreviousPlaylist != NULL;
	isPlaying = false;

	if (!fSchedule)
		return;

	isPlaying = fCurrentPlaylist != NULL;

	// make sure that we playback the correct playlist
	// at the correct offset for the given time of the day

	// get current daytime
	bigtime_t t = real_time_clock_usecs();
	time_t seconds = (time_t)(t / 1000000);
	bigtime_t offset = t - (bigtime_t)seconds * 1000000;
	tm time = *localtime(&seconds);

	// calc the current frame of the day since 0:00.00
	uint64 frame = (1000000LL
		* (time.tm_hour * 60 * 60 + time.tm_min * 60 + time.tm_sec)
		+ offset) * 25 / 1000000;

	AutoWriteLocker _(fLocker);

	if (fNavigationPlaylist) {
		// playback in navigation track
		int64 duration = fNavigationPlaylist->Duration();
		int64 nextStartFrame = fNavigationPlaylistStartFrame + duration;

		if (fPreviousNavigationPlaylist == NULL) {
			// switch from regular schedule
			fMasterPlaylist->SetSoloTrack(kNavigationTrack);
			// cleanup for the case that there was still something
			// on the navigation track
			TRACE_SP("cleaning up track starting at %lld (start)\n",
				currentFrame);

			_CleanupTrack(fMasterPlaylist, kNavigationTrack, currentFrame);
			nextStartFrame = currentFrame;
		} else if (fPreviousNavigationPlaylist != fNavigationPlaylist) {
			// switch to new navigation playlist
			// cleanup previous playlist
			TRACE_SP("cleaning up track starting at %lld (switch)\n",
				currentFrame);

			_CleanupTrack(fMasterPlaylist, kNavigationTrack, currentFrame);
			nextStartFrame = currentFrame;
		} else if (fPreviousNavigationPlaylist == fNavigationPlaylist) {
			// check if navigation playlist needs to be added
			// at current frame
			if (currentFrame >= nextStartFrame)
				nextStartFrame = currentFrame;
		}
		if (nextStartFrame == currentFrame) {
			TRACE_SP("adding navigation playlist at %lld, duration: %lld\n",
				nextStartFrame, duration);

			if (!_AddPlaylistToMasterPlaylist(fNavigationPlaylist,
				kNavigationTrack, nextStartFrame, duration)) {
				TRACE_SP("  -> FAILURE!\n");
				return;
			}
			fNavigationPlaylistStartFrame = nextStartFrame;
		}
		fPreviousNavigationPlaylist = fNavigationPlaylist;
	} else if (fPreviousNavigationPlaylist) {
		// switch back to schedule track
		fPreviousNavigationPlaylist = NULL;
		fMasterPlaylist->SetSoloTrack(kScheduleTrack);
	}

	// get current playlist from schedule
	int32 index = fSchedule->IndexAtFrame(frame);

	TRACE_SP("  frame: %lld -> index %ld of %ld\n", frame, index,
		fSchedule->CountItems());

	if (index == fCurrentScheduleIndex) {
		// no change in current playlist
		return;
	}

	ScheduleItem* item = fSchedule->ItemAt(index);
	Playlist* playlist = item != NULL ? item->Playlist() : NULL;

	// we need to add this playlist to the master playlist
	fPreviousPlaylist = fCurrentPlaylist;
	fCurrentPlaylist = playlist;
	fCurrentScheduleIndex = index;

	// TODO: cut off any previous items at the current frame
	// even if there is no new item/playlist (should be minor
	// annoyance if at all)

	// insert the next playlist if there is one
	if (!playlist || playlist->Duration() == 0)
		return;

	int64 startFrameOffset = item->StartFrame() - frame;
	int64 startFrame = currentFrame + startFrameOffset;

	TRACE_SP("  startFrame: %lld\n", startFrame);

	// make sure there is no item at or after the current frame
	_CleanupTrack(fMasterPlaylist, kScheduleTrack, startFrame);

	uint64 itemDuration = min_c(item->Duration(), (uint64)kWholeDayDuration);
	bool flexible = item->FlexibleDuration();
	bool firstPass = true;

	TRACE_SP("  itemDuration: %llu\n", itemDuration);

	while (startFrame < kWholeDayDuration && itemDuration > 0) {

		uint64 duration = min_c(itemDuration, playlist->Duration());

		TRACE_SP("    duration: %llu\n", duration);

		if (!flexible && duration < playlist->Duration() && duration < 25
			&& !firstPass) {
			// we are already looping
			print_warning("not adding another playlist item because "
				"duration below 25 frames (assuming glitch)\n");
			break;
		}
		if (startFrame >= 0) {
			if (!_AddPlaylistToMasterPlaylist(playlist, kScheduleTrack,
				startFrame, duration))
				return;
		}
		startFrame += duration;
		itemDuration -= duration;
		firstPass = false;
	}

	isPlaying = true;
}

// #pragma mark -

// _CleanupTrack
void
PlayerPlaybackNavigator::_CleanupTrack(Playlist* playlist, uint32 track,
	int64 startFrame)
{
	int32 count = playlist->CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		PlaylistItem* item = playlist->ItemAtFast(i);

		if (item->Track() != track)
			continue;

		if (item->StartFrame() >= startFrame) {
			// remove any items after the frame at which
			// we want to insert the next playlist
			TRACE_SP("delete item (item start: %lld, start: %lld)\n",
				item->EndFrame(), startFrame);

			delete playlist->RemoveItem(i);
		} else if (item->EndFrame() >= startFrame) {
			// shorten any items that begin before this
			// frame and extend after it
			TRACE_SP("shorten item (end: %lld, start: %lld, duration %lld -> "
				"%lld)\n", item->EndFrame(), startFrame, item->Duration(),
				startFrame - item->StartFrame());

			item->SetDuration(startFrame - item->StartFrame());
		} else if (item->EndFrame() < startFrame) {
			// the way items are added to the master playlist
			// we can stop processing now, items can only be sorted
			// by startFrame
			break;
		}
	}
}

// _AddPlaylistToMasterPlaylist
bool
PlayerPlaybackNavigator::_AddPlaylistToMasterPlaylist(Playlist* playlist,
	uint32 track, int64 startFrame, int64 duration)
{
	ClipPlaylistItem* clipItem = new (nothrow) ClipPlaylistItem(playlist,
		startFrame, track);
	if (!clipItem) {
		print_error("no memory to create playlist item\n");
		return false;
	}
	clipItem->SetDuration(duration);
		// don't add item before having set these properties to
		// avoid unnecessary notifications and calculations

	TRACE_SP("    adding item: %lld\n", startFrame);

	if (!fMasterPlaylist->AddItem(clipItem)) {
		print_error("no memory to add playlist item\n");
		delete clipItem;
		return false;
	}

	return true;
}

// _NavigationTimeout
void
PlayerPlaybackNavigator::_NavigationTimeout()
{
	fNavigationPlaylist = NULL;
}




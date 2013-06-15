/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_H
#define SCHEDULE_H

#include <List.h>
#include <String.h>

#include "ServerObject.h"
//#include "XMLStorable.h"

class BMessage;
class Playlist;
class ScheduleItem;
class ScheduleObserver;
class TrackProperties;
class OptionProperty;
class StringProperty;
class WeekDaysProperty;

class Schedule : public ServerObject/*, public XMLStorable*/ {
 public:
								Schedule();
								Schedule(const Schedule& other, bool deep);
	virtual						~Schedule();

	// Selectable
	virtual	void				SelectedChanged();

	// ServerObject interface
	virtual	status_t			SetTo(const ServerObject* other);
	virtual	bool				IsMetaDataOnly() const;
	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	virtual	status_t			Unarchive(const BMessage* from);

	// Schedule
			void				AddScheduleObserver(ScheduleObserver* observer);
			void				RemoveScheduleObserver(ScheduleObserver* observer);
			void				StartNotificationBlock();
			void				FinishNotificationBlock();

			Schedule*			Clone(bool deep) const;

			void				GetCurrentPlaylist(Playlist** playlist,
									uint64* startFrame) const;
									// returns the current playlist at
									// the frame of day
			Playlist*			NextPlaylist() const;
									// returns the next playlist so that
									// preloading of the contained clips
									// can take place in the background
			int32				IndexAtCurrentTime() const;
									// returns the ScheduleItem index
									// for the current time of the day

	// at which days this schedule should be active
			OptionProperty*		Type() const
									{ return fType; }
									// either weekly or at specific date
			WeekDaysProperty*	WeekDays() const
									{ return fWeekDays; }
			StringProperty*		Date() const
									{ return fDate; }
	// TODO: another way to represent "dates" would be a list
	// of strings. The strings could be interpreted with parse_data()
	// and relative date strings like "Monday" would be resolved just
	// like specific date strings like "24.12."

			void				SanitizeStartFrames();

			Playlist*			GeneratePlaylist(int64 startFrame,
									int64 endFrame,
									bool offsetToZero = true) const;

	// list manipulation
			int32				InsertIndexAtFrame(uint64 frame) const;
			int32				IndexAtFrame(uint64 frame) const;

			bool				AddItem(ScheduleItem* item);
			bool				AddItem(ScheduleItem* item,
										int32 index);
			ScheduleItem*		RemoveItem(int32 index);
			bool				RemoveItem(ScheduleItem* item);

			ScheduleItem*		ItemAt(int32 index) const;
			ScheduleItem*		ItemAtFast(int32 index) const;

			bool				HasItem(ScheduleItem* item) const;
			int32				IndexOf(ScheduleItem* item) const;

			int32				CountItems() const;

			void				MakeEmpty();

 protected:
								Schedule(const char* type);
 private:
								Schedule(const Schedule& other);

			void				_NotifyItemAdded(ScheduleItem* item,
												 int32 index);
			void				_NotifyItemRemoved(ScheduleItem* item);
			void				_NotifyItemsLayouted();
			void				_NotifyNotificationBlockStarted();
			void				_NotifyNotificationBlockFinished();

			int32				_CountFlexibleItemsInRange(int32 index) const;

 			BList				fItems;

			BList				fObservers;
			int32				fNotificationBlocks;

			OptionProperty*		fType;
			WeekDaysProperty*	fWeekDays;
			StringProperty*		fDate;

	mutable	bigtime_t			fLastTimeCheck;
	mutable	int32				fCachedIndexAtCurrentFrame;
};

class ScheduleNotificationBlock {
 public:
	ScheduleNotificationBlock(Schedule* schedule)
		: fSchedule(schedule)
	{
		fSchedule->StartNotificationBlock();
	}
	~ScheduleNotificationBlock()
	{
		fSchedule->FinishNotificationBlock();
	}
 private:
	Schedule* fSchedule;
};

#endif // SCHEDULE_H

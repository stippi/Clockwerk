/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_ITEM_H
#define SCHEDULE_ITEM_H

#include <Archivable.h>
#include <String.h>

#include "AffineTransform.h"
#include "PlaylistObserver.h"
#include "PropertyObject.h"
#include "Referencable.h"
#include "Selectable.h"

class Playlist;
class Schedule;
class ServerObjectManager;

class ScheduleItem : public PropertyObject,
					 public Selectable,
					 public BArchivable,
					 public PlaylistObserver,
					 public Referencable {
 public:
								ScheduleItem(::Playlist* playlist);
								ScheduleItem(const ScheduleItem& other,
									bool deep);
								ScheduleItem(BMessage* archive);
	virtual						~ScheduleItem();

	// Selectable interface
	virtual	void				SelectedChanged();

	// BArchivable interface
	static	BArchivable*		Instantiate(BMessage* archive);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	// PlaylistObserver interface
	virtual	void				DurationChanged(::Playlist* playlist,
									uint64 duration);

	// ScheduleItem
	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

	virtual	ScheduleItem*		Clone(bool deep) const;

			void				SetParent(Schedule* parent);
			Schedule*			Parent() const
									{ return fParent; }

			BString				Name() const;

			void				SetPlaylist(::Playlist* playlist);
			::Playlist*			Playlist() const
									{ return fPlaylist; }

			void				SetStartFrame(uint64 frameOfDay);
			uint64				StartFrame() const
									{ return fStartFrame; }
			void				SetDuration(uint64 frames);
			uint64				Duration() const
									{ return fDuration; }
			uint64				PreferredDuration() const;
			uint64				PlaylistDuration() const;
									// in frames

			void				SetExplicitRepeats(uint16 repeats);
			uint16				ExplicitRepeats() const
									{ return fExplicitRepeats; }
			float				Repeats() const;

			void				SetFlexibleStartFrame(bool flexible);
			bool				FlexibleStartFrame() const
									{ return fFlexibleStartFrame; }
									// wether duration is multiple
									// of playlist duration or flexible

			void				SetFlexibleDuration(bool flexible);
			bool				FlexibleDuration() const
									{ return fFlexibleDuration; }
									// wether duration is multiple
									// of playlist duration or flexible

			void				FilterStartFrame(uint64* frameOfDay) const;
			void				FilterDuration(uint64* duration) const;

 protected:
								ScheduleItem(const ScheduleItem& other);

 private:
			void				_CreateProperties();
			uint64				_PlaylistDuration() const;
			void				_AdjustDuration();

			Schedule*			fParent;

			::Playlist*			fPlaylist;

			uint64				fStartFrame;
			uint64				fDuration;
									// duration in frames
			uint16				fExplicitRepeats;

			bool				fFlexibleStartFrame : 1;
			bool				fFlexibleDuration : 1;
};

#endif // SCHEDULE_ITEM_H

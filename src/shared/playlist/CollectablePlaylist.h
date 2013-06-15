/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COLLECTABLE_PLAYLIST_H
#define COLLECTABLE_PLAYLIST_H


#include "StretchingPlaylist.h"


class CollectablePlaylist : public StretchingPlaylist {
 public:
								CollectablePlaylist();
								CollectablePlaylist(
									const CollectablePlaylist& other);
	virtual						~CollectablePlaylist();

	// CollectablePlaylist
			void				SetTypeMarker(const char* typeMarker);
			const char*			TypeMarker() const;

			void				SetSequenceIndex(int32 index);
			int32				SequenceIndex() const;

			void				SetStartDate(const char* date);
			const char*			StartDate() const;

			void				SetValidDayCount(int32 days);
			int32				ValidDays() const;

			bool				IsValidToday() const;

			uint64				PreferredDuration() const;
};

#endif // COLLECTABLE_PLAYLIST_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TRACK_HEADER_VIEW_H
#define TRACK_HEADER_VIEW_H

#include <String.h>
#include <View.h>

#include "Observer.h"

class CommandStack;
class Playlist;
class TimelineView;

class TrackHeaderView : public BView, public Observer {
 public:
								TrackHeaderView(BRect frame);
	virtual						~TrackHeaderView();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// TrackHeaderView
			void				SetPlaylist(Playlist* playlist);
			void				SetCommandStack(CommandStack* stack);
			void				SetTimelineView(TimelineView* view);

 private:
			Playlist*			fPlaylist;
			CommandStack*		fCommandStack;
			TimelineView*		fTimelineView;
			BString				fPlaylistName;
};

#endif // TRACK_HEADER_VIEW_H

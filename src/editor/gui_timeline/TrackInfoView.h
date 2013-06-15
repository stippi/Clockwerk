/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TRACK_INFO_VIEW_H
#define TRACK_INFO_VIEW_H

#include "InfoView.h"
#include "Observer.h"

class CommandStack;
class Playlist;
class TimelineView;

class TrackInfoView : public InfoView, public Observer {
 public:
								TrackInfoView(BRect frame);
	virtual						~TrackInfoView();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// TrackInfoView
			void				SetPlaylist(Playlist* playlist);
			void				SetCommandStack(CommandStack* stack);
			void				SetTimelineView(TimelineView* view);

 private:
			Playlist*			fPlaylist;
			CommandStack*		fCommandStack;
			TimelineView*		fTimelineView;
};

#endif // TRACK_INFO_VIEW_H

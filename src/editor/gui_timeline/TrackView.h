/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TRACK_VIEW_H
#define TRACK_VIEW_H

#include <View.h>

#include "PlaylistObserver.h"

class CommandStack;
class Playlist;
class TimelineView;

class TrackView : public BView, public PlaylistObserver {
 public:
								TrackView(BRect frame);
	virtual						~TrackView();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	// PlaylistObserver
	virtual	void				TrackPropertiesChanged(Playlist* playlist,
									uint32 track);
	virtual	void				TrackMoved(Playlist* playlist, uint32 oldIndex,
									uint32 newIndex);
	virtual	void				TrackInserted(Playlist* playlist, uint32 track);
	virtual	void				TrackRemoved(Playlist* playlist, uint32 track);

	// TrackView
			void				SetPlaylist(Playlist* playlist);
			void				SetCommandStack(CommandStack* stack);
			void				SetTimelineView(TimelineView* view);

 private:
			BRect				_TrackRect(uint32 track) const;
			BRect				_SoloRect(BRect trackRect) const;
			BRect				_MuteRect(BRect trackRect) const;
			void				_DrawButton(BRect frame,
											const char* label,
											float ascent,
											float width,
											rgb_color base,
											rgb_color fill,
											bool top);
			BString				_GetTrackName(uint32 track,
											  bool generate = true) const;
			void				_GenerateTrackName(BString& name,
												   uint32 track) const;
			void				_EditTrackName(uint32 track);

			Playlist*			fPlaylist;
			CommandStack*		fCommandStack;
			TimelineView*		fTimelineView;

			BFont				fFont;
			BFont				fButtonFont;

			float				fMuteWidth;
			float				fSoloWidth;
			float				fButtonWidth;

			float				fAscent;
			float				fSmallAscent;

			int32				fEditedTrackName;
									// used to prevent drawing the
									// name of the track at this index
									// (when it is being edited)

			int32				fDraggedTrack;
};

#endif // TRACK_VIEW_H

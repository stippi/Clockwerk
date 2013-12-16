/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIMELINE_VIEW_H
#define TIMELINE_VIEW_H

#include <List.h>

#include "Observer.h"
#include "PlaylistObserver.h"
#include "Scrollable.h"
#include "StateView.h"

class BClipboard;
class CurrentFrame;
class DisplayRange;
class DropAnticipationState;
class LoopMode;
class Manipulator;
class MultipleManipulatorState;
class NavigationInfoPanel;
class PlaybackManager;
class Playlist;
class PlaylistItem;
class Selectable;
class Selection;
class ServerObjectManager;
class TimelineTool;
class TimeView;
class TrackView;

enum {
	MSG_CUT_ITEMS				= 'ctim',
	MSG_REMOVE_ITEMS			= 'rmsi',
	MSG_DUPLICATE_ITEMS			= 'dpsi',

	MSG_ZOOM_OUT				= 'zmot',
	MSG_ZOOM_IN					= 'zmin',
};

enum {
	FOLLOW_MODE_OFF				= 0,
	FOLLOW_MODE_PAGE,
	FOLLOW_MODE_INCREMENTAL,
};

class TimelineView : public StateView, public Observer,
	public PlaylistObserver, public Scrollable {
public:
								TimelineView(const char* name);
	virtual						~TimelineView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

	virtual	void				Draw(BRect updateRect);

	// StateView interface
public:
	virtual	bool				MouseWheelChanged(BPoint where,
									float x, float y);
	virtual	void				NothingClicked(BPoint where, uint32 buttons,
									uint32 clicks);
	virtual	ViewState*			StateForDragMessage(
									const BMessage* dragMessage);

 protected:
	virtual	bool				_HandleKeyDown(const KeyEvent& event,
									BHandler* originalTarget);
	virtual	bool				_HandleKeyUp(const KeyEvent& event,
									BHandler* originalTarget);

	// Observer interface
 public:
	virtual	void				ObjectChanged(const Observable* object);

	// PlaylistObserver interface
	virtual	void				ItemAdded(::Playlist* playlist,
									PlaylistItem* item, int32 index);
	virtual	void				ItemRemoved(::Playlist* playlist,
									PlaylistItem* item);

	virtual	void				DurationChanged(::Playlist* playlist,
									uint64 duration);
	virtual	void				MaxTrackChanged(::Playlist* playlist,
									uint32 maxTrack);

	// Scrollable interface
 protected:
	virtual	void				DataRectChanged(BRect oldDataRect,
												BRect newDataRect);
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);
	virtual	void				VisibleSizeChanged(float oldWidth,
												   float oldHeight,
												   float newWidth,
												   float newHeight);
	// TimelineView
 public:
			void				SetPlaylist(::Playlist* list);
			::Playlist*			Playlist() const
									{ return fPlaylist; }

			void				SetClipLibrary(ServerObjectManager* library);

			void				SetCurrentFrame(::CurrentFrame* frame);
			::CurrentFrame*		CurrentFrameObject() const
									{ return fCurrentFrame; }
			void				SetDisplayRange(DisplayRange* range);
			void				SetPlaybackManager(::PlaybackManager* manager);
			::PlaybackManager*	PlaybackManager() const
									{ return fPlaybackManager; }
			void				SetLoopMode(LoopMode* mode);
			void				SetTimeView(TimeView* view);
			void				SetTrackView(TrackView* view);
			void				SetAutoscrollEnabled(bool enabled);
			void				SetAutoscrollFollowMode(uint32 mode,
														bool followNow = true);

			void				SetSelection(::Selection* selection);
			::Selection*		Selection() const
									{ return fSelection; }

			void				SetTool(TimelineTool* tool);

	// helper function so that the Selection doesn't
	// need to be known to everybody
			void				Select(Selectable* object);

			void				CopySelectedItems();
			void				RemoveItem(PlaylistItem* item);
			void				CutSelectedItems();
			void				RemoveSelectedItems();
			void				DuplicateSelectedItems();

			void				Paste();
			BClipboard*			Clipboard() const
									{ return fClipboard; }

	// view state
			uint32				TrackHeight() const;

			int64				StartFrame() const;
			double				FramesPerPixel() const;

			int64				FrameForPos(float x) const;
			float				PosForFrame(int64 frame) const;
			uint32				TrackForPos(float y) const;

	virtual	double				ZoomLevel() const
									{ return fZoomLevel; }

			bool				IsPaused() const;
			int64				CurrentFrame() const;
 private:
			void				_DeleteManipulators();
			void				_MakeManipulators();
			void				_InvalidateManipulators();

			BRect				_CurrentFrameMarkerRect() const;
			BRect				_TimelineRect() const;

			PlaylistItem**		_GetSelectedItems(int32& count) const;

			double				_NetZoomOutLevel(double zoom) const;
			double				_NetZoomInLevel(double zoom) const;
			void				_SetZoom(float zoomLevel);

			void				_FollowPlaybackFrame();

			void				_ShowPopupMenuForEmptiness(BPoint where);

	::Playlist*					fPlaylist;
	ServerObjectManager*		fClipLibrary;
	::PlaybackManager*			fPlaybackManager;
	::CurrentFrame*				fCurrentFrame;
	int64						fCurrentFrameMarker;

	DisplayRange*				fDisplayRange;
	LoopMode*					fLoopMode;
	double						fZoomLevel;
	bool						fZooming;
	uint64						fDisplayedFrames;
	bool						fAutoscroll;
	uint32						fAutoscrollFollowMode;

	::Selection*				fSelection;
	BClipboard*					fClipboard;

	TimelineTool*				fTool;
	MultipleManipulatorState*	fDefaultState;
	DropAnticipationState*		fInsertDropAnticipationState;
	DropAnticipationState*		fReplaceDropAnticipationState;

	TimeView*					fTimeView;
	TrackView*					fTrackView;
};

#endif // TIMELINE_VIEW_H

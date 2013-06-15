/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIME_VIEW_H
#define TIME_VIEW_H

#include <List.h>
#include <View.h>

#include "Observer.h"
#include "PropertyListView.h"

class BMessageRunner;
class Command;
class CommandStack;
class CurrentFrame;
class DisplayRange;
class KeyFrame;
class LoopMode;
class PropertyAnimator;
class PlaylistItem;
class TimelineView;

class TimeView : public BView, public Observer,
	public SelectedPropertyListener {
public:
								TimeView(CurrentFrame* frame);
	virtual						~TimeView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// SelectedPropertyListener interface
	virtual	void				PropertyObjectSet(PropertyObject* object);
	virtual	void				PropertySelected(Property* p);

	// TimeView
			void				SetDisplayRange(DisplayRange* range);
			int64				FirstFrame() const;
			int64				LastFrame() const;

			void				SetPlaybackRange(DisplayRange* range);
			void				SetLoopMode(LoopMode* mode);

			void				SetFramesPerSecond(float fps);

			void				SetCommandStack(CommandStack* stack);

			void				SetInsets(int32 left, int32 right);
			void				SetTimelineView(TimelineView* view);

 private:
			BString				_TimeStringForFrame(int64 frame,
													bool ticks);

			float				_PosForFrame(int64 frame) const;
			int64				_FrameForPos(float pos) const;

			BRect				_MarkerRect() const;
			void				_SetMarker(float pos);
			BRect				_PlaybackRangeRect() const;

			void				_SetPlaylistItem(PlaylistItem* item);
			void				_SetPropertyAnimator(
									PropertyAnimator* animator);

			void				_RebuildKeyPoints();
			void				_MoveKeyFrames(int32 offset);

			void				_SetKeyFrame(float pos);

			void				_AttachKeyFrame();
			void				_DetachKeyFrame(BPoint where);

			void				_SetAutoscrollEnabled(bool enable);


			CurrentFrame*		fCurrentFrame;
			int64				fMarkerPos;
			uint32				fTracking;
			BMessageRunner*		fAutoscrollPulse;

			DisplayRange*		fDisplayRange;
			int64				fFirstFrame;
			int64				fLastFrame;

			DisplayRange*		fPlaybackRange;
			int64				fFirstPlaybackFrame;
			int64				fLastPlaybackFrame;
			int64				fDragStartFrame;

			LoopMode*			fLoopMode;
			bool				fRangeEnabled;

			int32				fLeftInset;
			int32				fRightInset;

			float				fFPS;
			bool				fDisplayTicks;

			PropertyAnimator*	fAnimator;
			PlaylistItem*		fItem;
			int64				fKeyFrameOffset;

			BList				fKeyPoints;
			KeyFrame*			fDraggedKey;
			float				fDraggedKeyOffset;
			bool				fKeyFrameRemoved;

			CommandStack*		fCommandStack;
			Command*			fKeyFrameCommand;

			TimelineView*		fTimelineView;
};

#endif // TIME_VIEW_H

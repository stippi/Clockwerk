/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TOP_VIEW_H
#define TOP_VIEW_H

#include <View.h>

class BSplitView;
class EditorVideoView;

class TopView : public BView {
 public:
								TopView(BRect frame);
	virtual						~TopView();

	// BView interface
	virtual	void				FrameResized(float width, float height);

	// TopView
			void				AddMenuBar(BView* view);
			void				AddClipListGroup(BView* view);
			void				AddPropertyListGroup(BView* view);
			void				AddIconBar(BView* view);
			void				AddStageIconBar(BView* view);
			void				AddTimelineGroup(BView* view);
			void				AddTrackGroup(BView* view);
			void				AddVideoView(EditorVideoView* view,
									BView* container);
			void				AddTransportGroup(BView* view);

			void				SetVideoSize(uint32 nativeWidth, float aspect);
			void				SetVideoScale(float scale);
			float				VideoScale() const
									{ return fVideoScale; }
			void				SetListProportion(float proportion);
			float				ListProportion() const
									{ return fListProportion; }
			void				SetTrackProportion(float proportion);
			float				TrackProportion() const
									{ return fTrackProportion; }

private:
	class Splitter;
	class ListSplitter;
	class VideoSplitter;
	class TrackSplitter;

			void				_AddChild(BView* view);
			void				_Relayout();

			BView*				fMenuBar;
			BView*				fClipListGroup;
			BView*				fPropertyListGroup;
			BView*				fIconBar;
			BView*				fStageIconBar;
			BView*				fTimelineGroup;
			BView*				fTrackGroup;
			BView*				fVideoScrollView;
			EditorVideoView*	fVideoView;
			BView*				fTransportGroup;

			BSplitView*			fListGroup;
			ListSplitter*		fListSplitter;
			VideoSplitter*		fVideoSplitter;
			TrackSplitter*		fTrackSplitter;

			uint32				fNativeVideoWidth;
			float				fVideoAspect;

			float				fVideoScale;
			float				fListProportion;
			float				fTrackProportion;

			float				fOriginalIconBarWidth;
};

#endif // TOP_VIEW_H

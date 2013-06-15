/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_TOP_VIEW_H
#define SCHEDULE_TOP_VIEW_H

#include <View.h>

class ScheduleTopView : public BView {
 public:
								ScheduleTopView(BRect frame);
	virtual						~ScheduleTopView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);

	// ScheduleTopView
			void				AddMenuBar(BView* view);
			void				AddStatusBar(BView* view);
			void				AddScheduleListGroup(BView* view);
			void				AddPlaylistListGroup(BView* view);
			void				AddPropertyGroup(BView* view);
			void				AddScheduleGroup(BView* view);

			void				SetListProportion(float proportion);
			float				ListProportion() const
									{ return fListProportion; }

 private:
	class Splitter;
	class ListSplitter;

			void				_AddChild(BView* view);
			void				_Relayout();

			BView*				fMenuBar;
			BView*				fStatusBar;
			BView*				fScheduleListGroup;
			BView*				fPlaylistListGroup;
			BView*				fPropertyGroup;
			BView*				fScheduleGroup;

			ListSplitter*		fListSplitter;

			float				fListProportion;
};

#endif // SCHEDULE_TOP_VIEW_H

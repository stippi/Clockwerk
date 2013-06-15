/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_VIEW_H
#define SCHEDULE_VIEW_H

#include <List.h>

#include "BackBufferedStateView.h"
#include "Observer.h"
#include "ScheduleObserver.h"
#include "Scrollable.h"

class BClipboard;
class DropAnticipationState;
class Manipulator;
class MultipleManipulatorState;
class Playlist;
class Schedule;
class ScheduleItem;
class Selectable;
class Selection;

enum {
	MSG_REMOVE_ITEM				= 'rmim',
	MSG_REMOVE_ITEMS			= 'rmsi',
	MSG_DUPLICATE_ITEMS			= 'dpsi',
};

class ScheduleView : public BackBufferedStateView,
					 public Observer,
					 public ScheduleObserver,
					 public Scrollable {
 public:
								ScheduleView(BRect frame,
											 const char* name);
	virtual						~ScheduleView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

	// StateView interface
	virtual	void				NothingClicked(BPoint where, uint32 buttons,
									uint32 clicks);

	virtual	bool				MouseWheelChanged(BPoint where,
									float x, float y);

	virtual	ViewState*			StateForDragMessage(const BMessage* dragMessage);

 protected:
	virtual	bool				_HandleKeyDown(const KeyEvent& event,
									BHandler* originalTarget);
	virtual	bool				_HandleKeyUp(const KeyEvent& event,
									BHandler* originalTarget);

	// BackBufferedStateView interface
 protected:
	virtual	void				DrawBackgroundInto(BView* view,
									BRect updateRect);
	virtual	void				DrawInto(BView* view, BRect updateRect);

	// Observer interface
 public:
	virtual	void				ObjectChanged(const Observable* object);

	// ScheduleObserver interface
	virtual	void				ItemAdded(ScheduleItem* item, int32 index);
	virtual	void				ItemRemoved(ScheduleItem* item);
	virtual	void				ItemsLayouted();

	virtual	void				NotificationBlockStarted();
	virtual	void				NotificationBlockFinished();

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
	// ScheduleView
 public:
	// layouting schedule items
			float				HeightForTime(uint32 seconds) const;
			uint32				TimeForHeight(float height) const;
			uint64				FrameForHeight(float height,
									ScheduleItem* item) const;
			float				TextLineHeight() const
									{ return fTextLineHeight; }

			BRect				LayoutScheduleItem(ScheduleItem* item) const;
			BRect				LayoutScheduleItem(uint64 startFrame,
									uint64 endFrame) const;

			void				SetSchedule(::Schedule* list);
			::Schedule*			Schedule() const
									{ return fSchedule; }

			void				SetSelection(::Selection* selection);
			::Selection*		Selection() const
									{ return fSelection; }

			void				HighlightPlaylist(const Playlist* playlist);
			void				SelectAllItems(const Playlist* playlist);

	// helper function so that the Selection doesn't
	// need to be known to everybody
			void				Select(Selectable* object);

			void				CopySelectedItems();
			void				RemoveItem(ScheduleItem* item);
			void				RemoveSelectedItems();
			void				DuplicateSelectedItems();

			void				Paste();
			BClipboard*			Clipboard() const
									{ return fClipboard; }
 private:
			float				_IdealHeightForTime(uint32 seconds) const;
			uint32				_IdealTimeForHeight(float height) const;

			void				_ItemAdded(ScheduleItem* item, int32 index);
			void				_ItemRemoved(ScheduleItem* item);
			void				_ItemsLayouted();

			void				_DeleteManipulators();
			void				_MakeManipulators();
			void				_InvalidateManipulators();

			BRect				_ScheduleRect() const;

			void				_DrawTimeColumn(BView* view,
									BRect updateRect);
			void				_DrawCenteredMessage(BView* view,
									BRect bounds, const char* message);

			ScheduleItem**		_GetSelectedItems(int32& count) const;

			void				_ClearProblemRanges();
			void				_RebuildProblemRanges();

	::Schedule*					fSchedule;

	::Selection*				fSelection;
	BClipboard*					fClipboard;

	MultipleManipulatorState*	fDefaultState;
	DropAnticipationState*		fInsertDropAnticipationState;
//	DropAnticipationState*		fReplaceDropAnticipationState;

	float						fTextLineHeight;
	float						fTextAscent;

	bool						fInNotificationBlock;

	const Playlist*				fHighlightPlaylist;

	struct problem_range {
		problem_range(uint64 s, uint64 e, bool g)
			: startFrame(s)
			, endFrame(e)
			, gap(g)
		{}

		uint64	startFrame;
		uint64	endFrame;
		bool	gap;
	};

	BList						fProblemRanges;
};

#endif // SCHEDULE_VIEW_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleView.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <Clipboard.h>
#include <Message.h>
#include <Region.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <Window.h>

#include "common.h"
#include "support.h"
#include "support_date.h"

#include "InsertScheduleItemDropState.h"
#include "MultipleManipulatorState.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "ScheduleItemManipulator.h"
#include "RemoveScheduleItemsCommand.h"
#include "RWLocker.h"
#include "Scroller.h"
#include "Selectable.h"
#include "Selection.h"
#include "TimeProperty.h"

using std::nothrow;

#define TIME_COLUMN_WIDTH 120
#define SCHEDULE_COLUMN_WIDTH 350
#define TEXT_LINES_PER_HOUR 2

enum {
	MSG_SCHEDULE_ITEM_ADDED		= 'siad',
	MSG_SCHEDULE_ITEM_REMOVED	= 'sirm',
	MSG_SCHEDULE_ITEMS_LAYOUTED	= 'silt',

	MSG_INVALIDATE_MANIPULATORS	= 'invm'
};

// constructor
ScheduleView::ScheduleView(BRect frame, const char* name)
	: BackBufferedStateView(frame, name, B_FOLLOW_ALL, B_WILL_DRAW)
	, fSchedule(NULL)

	, fSelection(NULL)
	, fClipboard(be_clipboard)

	, fDefaultState(new MultipleManipulatorState(this))
	, fInsertDropAnticipationState(new InsertScheduleItemDropState(this))
//	, fReplaceDropAnticipationState(new ReplaceClipDropState(this))

	, fTextLineHeight(15.0)
	, fTextAscent(10.0)

	, fInNotificationBlock(false)

	, fHighlightPlaylist(NULL)
	, fProblemRanges(32)
{
	SetLowColor(180, 180, 180, 255);
	SyncGraphicsState();
}

// destructor
ScheduleView::~ScheduleView()
{
	_DeleteManipulators();

	delete fDefaultState;
	delete fInsertDropAnticipationState;
//	delete fReplaceDropAnticipationState;

	_DeleteManipulators();

	if (fSchedule) {
		fSchedule->RemoveScheduleObserver(this);
		int32 count = fSchedule->CountItems();
		for (int32 i = 0; i < count; i++)
			fSchedule->ItemAtFast(i)->RemoveObserver(this);
	}

	_ClearProblemRanges();
}

// #pragma mark -

// AttachedToWindow
void
ScheduleView::AttachedToWindow()
{
	BackBufferedStateView::AttachedToWindow();

	SetState(fDefaultState);
	MakeFocus();

	// cache text line height
	font_height fh;
	GetFontHeight(&fh);
	fTextLineHeight = ceilf(fh.ascent + fh.descent + fh.ascent * 0.8);
	fTextAscent = fh.ascent;

	SetDataRect(_ScheduleRect());
}

// DetachedFromWindow
void
ScheduleView::DetachedFromWindow()
{
	SetState(NULL);

	BackBufferedStateView::DetachedFromWindow();
}

// MessageReceived
void
ScheduleView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_COPY:
			CopySelectedItems();
			break;
		case B_CUT:
			CopySelectedItems();
			RemoveSelectedItems();
			break;
		case B_PASTE:
			Paste();
			break;

		case MSG_REMOVE_ITEM: {
			ScheduleItem* item;
			if (message->FindPointer("item", (void**)&item) == B_OK)
				RemoveItem(item);
			break;
		}

		case MSG_REMOVE_ITEMS:
			RemoveSelectedItems();
			break;
		case MSG_DUPLICATE_ITEMS:
			DuplicateSelectedItems();
			break;

		case MSG_SCHEDULE_ITEM_ADDED: {
			ScheduleItem* item;
			int32 index;
			if (message->FindPointer("item", (void**)&item) == B_OK
				&& message->FindInt32("index", &index) == B_OK)
				_ItemAdded(item, index);
			break;
		}
		case MSG_SCHEDULE_ITEM_REMOVED: {
			ScheduleItem* item;
			if (message->FindPointer("item", (void**)&item) == B_OK)
				_ItemRemoved(item);
			break;
		}

		case MSG_SCHEDULE_ITEMS_LAYOUTED:
			_ItemsLayouted();
			break;

		case MSG_INVALIDATE_MANIPULATORS:
			_InvalidateManipulators();
			break;

		default:
			BackBufferedStateView::MessageReceived(message);
			break;
	}
}

// MouseDown
void
ScheduleView::MouseDown(BPoint where)
{
	MakeFocus(true);
	BackBufferedStateView::MouseDown(where);
}

// MouseUp
void
ScheduleView::MouseUp(BPoint where)
{
	BackBufferedStateView::MouseUp(where);
}

// NothingClicked
void
ScheduleView::NothingClicked(BPoint where, uint32 buttons, uint32 clicks)
{
	if (fSelection)
		fSelection->DeselectAll();
}

// MouseWheelChanged
bool
ScheduleView::MouseWheelChanged(BPoint where, float x, float y)
{
	BPoint scrollOffset = ScrollOffset();
	scrollOffset.x += x * fTextLineHeight * 2;
	scrollOffset.y += y * fTextLineHeight * 2;
	SetScrollOffset(scrollOffset);
	return true;
}

// #pragma mark -

// DrawBackgroundInto
void
ScheduleView::DrawBackgroundInto(BView* view, BRect updateRect)
{
	if (fSchedule) {
		updateRect.left = max_c(updateRect.left, TIME_COLUMN_WIDTH - 1);
		if (updateRect.IsValid()) {
			view->FillRect(updateRect, B_SOLID_LOW);
		}
	} else
		BackBufferedStateView::DrawBackgroundInto(view, updateRect);
}

// DrawInto
void
ScheduleView::DrawInto(BView* view, BRect updateRect)
{
	if (fSchedule) {
		_DrawTimeColumn(view, updateRect);
		if (fSchedule->CountItems() == 0) {
			BRect bounds(view->Bounds());
			bounds.left += TIME_COLUMN_WIDTH;
			_DrawCenteredMessage(view, bounds, "<drag playlists here>");
		}
	} else {
		_DrawCenteredMessage(view, view->Bounds(), "<select a schedule>");
	}

	BackBufferedStateView::DrawInto(view, updateRect);
}

// #pragma mark -

// ObjectChanged
void
ScheduleView::ObjectChanged(const Observable* object)
{
	const ScheduleItem* item = dynamic_cast<const ScheduleItem*>(object);
	if (item && !fInNotificationBlock) {
		// invalidate all manipulators since relayouting is necessary
		if (!Looper())
			_InvalidateManipulators();
		else
			Looper()->PostMessage(MSG_INVALIDATE_MANIPULATORS, this);
	}
}

// #pragma mark -

// ItemAdded
void
ScheduleView::ItemAdded(ScheduleItem* item, int32 index)
{
	if (!Looper()) {
		_ItemAdded(item, index);
		return;
	}

	BMessage message(MSG_SCHEDULE_ITEM_ADDED);
	message.AddPointer("item", item);
	message.AddInt32("index", index);
	Looper()->PostMessage(&message, this);
}

// ItemRemoved
void
ScheduleView::ItemRemoved(ScheduleItem* item)
{
	if (!Looper()) {
		_ItemRemoved(item);
		return;
	}

	BMessage message(MSG_SCHEDULE_ITEM_REMOVED);
	message.AddPointer("item", item);
	Looper()->PostMessage(&message, this);
}

// ItemsLayouted
void
ScheduleView::ItemsLayouted()
{
	if (!Looper())
		_ItemsLayouted();
	else
		Looper()->PostMessage(MSG_SCHEDULE_ITEMS_LAYOUTED, this);
}

// NotificationBlockStarted
void
ScheduleView::NotificationBlockStarted()
{
	fInNotificationBlock = true;
}

// NotificationBlockFinished
void
ScheduleView::NotificationBlockFinished()
{
	fInNotificationBlock = false;

	if (!Looper())
		_InvalidateManipulators();
	else
		Looper()->PostMessage(MSG_INVALIDATE_MANIPULATORS, this);
}

// #pragma mark -

// DataRectChanged
void
ScheduleView::DataRectChanged(BRect oldDataRect, BRect newDataRect)
{
}

// ScrollOffsetChanged
void
ScheduleView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	BPoint offset = newOffset - oldOffset;
	ScrollBy(offset.x, offset.y);
}

// VisibleSizeChanged
void
ScheduleView::VisibleSizeChanged(float oldWidth, float oldHeight,
								 float newWidth, float newHeight)
{
}

// #pragma mark -

// StateForDragMessage
ViewState*
ScheduleView::StateForDragMessage(const BMessage* dragMessage)
{
	if (!fSchedule)
		return NULL;
//	if (modifiers() & B_COMMAND_KEY) {
//		if (fReplaceDropAnticipationState->WouldAcceptDragMessage(dragMessage))
//			return fReplaceDropAnticipationState;
//	}
	return fInsertDropAnticipationState;
	return NULL;
}

// #pragma mark -

// _HandleKeyDown
bool
ScheduleView::_HandleKeyDown(const KeyEvent& event, BHandler* originalTarget)
{
	if (dynamic_cast<BTextView*>(originalTarget))
		return false;

	bool handled = true;
	switch (event.key) {
		case B_DELETE:
			RemoveSelectedItems();
			break;
		default:
			handled = false;
	}
	return handled;
}

// _HandleKeyUp
bool
ScheduleView::_HandleKeyUp(const KeyEvent& event, BHandler* originalTarget)
{
	return false;
}

// #pragma mark -

// SetSchedule
void
ScheduleView::SetSchedule(::Schedule* schedule)
{
	if (fSchedule == schedule)
		return;

	if (fSchedule) {
		fSchedule->RemoveScheduleObserver(this);
		int32 count = fSchedule->CountItems();
		for (int32 i = 0; i < count; i++)
			fSchedule->ItemAtFast(i)->RemoveObserver(this);
	}

	fSchedule = schedule;

	if (fSchedule) {
		fSchedule->AddScheduleObserver(this);
		int32 count = fSchedule->CountItems();
		for (int32 i = 0; i < count; i++)
			fSchedule->ItemAtFast(i)->AddObserver(this);
	}

	_MakeManipulators();

	SetDataRect(_ScheduleRect());
}

// #pragma mark -

// HeightForTime
float
ScheduleView::HeightForTime(uint32 seconds) const
{
	uint64 frame = (uint64)seconds * 25;
	float additionalItemHeight = 0.0;
	if (fSchedule) {
		int32 count = fSchedule->CountItems();
		for (int32 i = 0; i < count; i++) {
			ScheduleItem* item = fSchedule->ItemAtFast(i);
			if (item->StartFrame() < frame)
				additionalItemHeight += fTextLineHeight;
		}
	}

	return _IdealHeightForTime(seconds) + additionalItemHeight;
}

// TimeForHeight
uint32
ScheduleView::TimeForHeight(float height) const
{
	if (height < 0)
		return 0;
	float additionalItemHeight = 0.0;

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator = fDefaultState->ManipulatorAtFast(i);
		if (manipulator->Bounds().top < height)
			additionalItemHeight += fTextLineHeight;
	}

	return _IdealTimeForHeight(height - additionalItemHeight);
}

// FrameForHeight
uint64
ScheduleView::FrameForHeight(float height, ScheduleItem* _item) const
{
	if (height < 0)
		return 0;
	float additionalItemHeight = fTextLineHeight;

	int32 count = fSchedule->CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* item = fSchedule->ItemAtFast(i);
		if (item->StartFrame() < _item->StartFrame())
			additionalItemHeight += fTextLineHeight;
	}

	return (uint64)(_IdealTimeForHeight(height - additionalItemHeight)) * 25;
}

// LayoutScheduleItem
BRect
ScheduleView::LayoutScheduleItem(ScheduleItem* item) const
{
	BRect r;
	if (!item)
		return r;

	uint64 startFrame = item->StartFrame();
	uint64 endFrame = startFrame + item->Duration();
	return LayoutScheduleItem(startFrame, endFrame);
}

// LayoutScheduleItem
BRect
ScheduleView::LayoutScheduleItem(uint64 startFrame, uint64 endFrame) const
{
	BRect r;
	r.left = TIME_COLUMN_WIDTH;
	r.right = r.left + SCHEDULE_COLUMN_WIDTH;
	r.top = HeightForTime((uint32)(startFrame / 25));
	float offset = r.top - _IdealHeightForTime((uint32)(startFrame / 25));
	r.bottom = _IdealHeightForTime((uint32)(endFrame / 25))
		+ fTextLineHeight + offset;
	return r;
}

// #pragma mark -

// SetSelection
void
ScheduleView::SetSelection(::Selection* selection)
{
	fSelection = selection;
}

// Select
void
ScheduleView::Select(Selectable* object)
{
	if (!fSelection)
		return;

	if (modifiers() & B_SHIFT_KEY) {
		// toggle selection state of object
		// without changing the state of other objects
		if (object->IsSelected())
			fSelection->Deselect(object);
		else
			fSelection->Select(object, true);
	} else {
		// just select the object
		fSelection->Select(object, false);
	}
}

// HighlightPlaylist
void
ScheduleView::HighlightPlaylist(const Playlist* playlist)
{
	if (fHighlightPlaylist == playlist)
		return;

	fHighlightPlaylist = playlist;

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		ScheduleItemManipulator* manipulator =
			(ScheduleItemManipulator*)fDefaultState->ManipulatorAtFast(i);
		if (ScheduleItem* item = manipulator->Item())
			manipulator->SetHighlighted(item->Playlist() == playlist);
	}
}

// SelectAllItems
void
ScheduleView::SelectAllItems(const Playlist* playlist)
{
	if (!fSelection)
		return;

	fSelection->DeselectAll();

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		ScheduleItemManipulator* manipulator =
			(ScheduleItemManipulator*)fDefaultState->ManipulatorAtFast(i);
		if (ScheduleItem* item = manipulator->Item()) {
			if (item->Playlist() == playlist)
				fSelection->Select(item, true);
		}
	}
}

// #pragma mark -

// CopySelectedItems
void
ScheduleView::CopySelectedItems()
{
printf("ScheduleView::CopySelectedItems()\n");
//	int32 count;
//	ScheduleItem** items = _GetSelectedItems(count);
//
//	if (count <= 0) {
//		printf("no item selected\n");
//		return;
//	}
//
//	BMessage itemsArchive;
//	for (int32 i = 0; i < count; i++) {
//		BMessage archive;
//		if (items[i]->Archive(&archive, true) < B_OK) {
//			printf("error archiving item %ld\n", i);
//			return;
//		}
//
//		if (itemsArchive.AddMessage("item", &archive) < B_OK) {
//			printf("error adding archive %ld to clipboard\n", i);
//			return;
//		}
//	}
//
//	if (!fClipboard->Lock()) {
//		printf("error locking clipboard\n");
//		return;
//	}
//
//	if (fClipboard->Clear() == B_OK) {
//
//		BMessage* data = fClipboard->Data();
//		data->AddMessage("clockwerk:playlist_items", &itemsArchive);
//
//		fClipboard->Commit();
//	} else {
//		printf("error clearing clipboard\n");
//	}
//
//	fClipboard->Unlock();
}

// RemoveItem
void
ScheduleView::RemoveItem(ScheduleItem* item)
{
	if (!item || !fSchedule || !fSelection)
		return;

	AutoWriteLocker locker(Locker());
	if (Locker() && !locker.IsLocked())
		return;

	ScheduleItem** items = new (nothrow) ScheduleItem*[1];
	if (!items)
		return;

	items[0] = item;
	RemoveScheduleItemsCommand* command
		= new (nothrow) RemoveScheduleItemsCommand(fSchedule,
			items, 1, fSelection);

	PerformCommand(command);
}

// RemoveSelectedItems
void
ScheduleView::RemoveSelectedItems()
{
	if (!fSchedule || !fSelection)
		return;

	AutoWriteLocker locker(Locker());
	if (Locker() && !locker.IsLocked())
		return;

	int32 count;
	ScheduleItem** items = _GetSelectedItems(count);

	RemoveScheduleItemsCommand* command
		= new (nothrow) RemoveScheduleItemsCommand(fSchedule,
			items, count, fSelection);

	PerformCommand(command);
}

// DuplicateSelectedItems
void
ScheduleView::DuplicateSelectedItems()
{
printf("ScheduleView::DuplicateSelectedItems()\n");
//	if (!fSchedule || !fSelection)
//		return;
//
//	AutoWriteLocker locker(Locker());
//	if (Locker() && !locker.IsLocked())
//		return;
//
//	int32 count;
//	ScheduleItem** items = _GetSelectedItems(count);
//
//	DuplicateItemsCommand* command = new (nothrow) DuplicateItemsCommand(
//		fSchedule, (const ScheduleItem**)items, count, fSelection);
//
//	delete[] items;
//
//	PerformCommand(command);
}

// Paste
void
ScheduleView::Paste()
{
printf("ScheduleView::Paste()\n");
//	if (!fSchedule || !fClipLibrary || !fClipboard->Lock())
//		return;
//
//	BMessage itemsArchive;
//	BMessage* data = fClipboard->Data();
//	data->FindMessage("clockwerk:playlist_items", &itemsArchive);
//
//	fClipboard->Unlock();
//
//	if (itemsArchive.IsEmpty())
//		return;
//
//	BList items;
//
//	BMessage archive;
//	for (int32 i = 0; itemsArchive.FindMessage("item", i, &archive) == B_OK;
//		 i++) {
//		BArchivable* archivable = instantiate_object(&archive);
//		ScheduleItem* item = dynamic_cast<ScheduleItem*>(archivable);
//		if (item) {
//			item->ResolveDependencies(fClipLibrary);
//			if (!items.AddItem((void*)item))
//				delete item;
//		} else {
//			printf("failed to instantiate ScheduleItem from archive %ld\n", i);
//		}
//	}
//
//	// TODO: ...
//	PerformCommand(new InsertCommand(fSchedule, fSelection,
//		(ScheduleItem**)items.Items(), items.CountItems(), fCurrentFrameMarker,
//		fSchedule->MaxTrack() + 1));
}

// #pragma mark -

// _IdealHeightForTime
float
ScheduleView::_IdealHeightForTime(uint32 seconds) const
{
	uint32 hours = seconds / (60 * 60);
	uint32 remainingSeconds = seconds - hours * 60 * 60;
	float hourHeight = TEXT_LINES_PER_HOUR * fTextLineHeight;
	float height = hours * hourHeight;
	height += remainingSeconds * hourHeight / (60 * 60);

	return floorf(height + 0.5);
}

// _IdealTimeForHeight
uint32
ScheduleView::_IdealTimeForHeight(float height) const
{
	if (height < 0.0)
		return 0;
	float hourHeight = TEXT_LINES_PER_HOUR * fTextLineHeight;
	uint32 seconds = uint32(60 * 60 * height / hourHeight);
	return seconds;
}

// #pragma mark -

// _ItemAdded
void
ScheduleView::_ItemAdded(ScheduleItem* item, int32 index)
{
	AutoReadLocker _(fLocker);

	ScheduleItemManipulator* manipulator
		= new (nothrow) ScheduleItemManipulator(item);
	if (manipulator) {
		if (!fDefaultState->AddManipulator(manipulator)) {
			delete manipulator;
			print_error("ScheduleView::_MakeManipulators() - "
				"unable to add manipulator\n");
			return;
		} else {
			manipulator->SetHighlighted(item->Playlist() == fHighlightPlaylist,
				false);
			if (!fInNotificationBlock)
				Invalidate(manipulator->Bounds());
		}
	}

	item->AddObserver(this);
}

// _ItemRemoved
void
ScheduleView::_ItemRemoved(ScheduleItem* item)
{
	AutoReadLocker _(fLocker);

	item->RemoveObserver(this);

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		ScheduleItemManipulator* manipulator =
			(ScheduleItemManipulator*)fDefaultState->ManipulatorAtFast(i);
		if (manipulator->Item() == item
			&& fDefaultState->RemoveManipulator(i) == manipulator) {
			delete manipulator;
			break;
		}
	}

	if (!fInNotificationBlock)
		_InvalidateManipulators();
}

// _ItemsLayouted
void
ScheduleView::_ItemsLayouted()
{
	SetDataRect(_ScheduleRect());
	_RebuildProblemRanges();
}

// #pragma mark -

// _DeleteManipulators
void
ScheduleView::_DeleteManipulators()
{
	if (LockLooper()) {
		fDefaultState->DeleteManipulators();
		UnlockLooper();
	}
}

// _MakeManipulators
void
ScheduleView::_MakeManipulators()
{
	_DeleteManipulators();
	Invalidate();

	if (!fSchedule)
		return;

	if (fLocker && !fLocker->ReadLock())
		return;

	int32 count = fSchedule->CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* item = fSchedule->ItemAtFast(i);
		ScheduleItemManipulator* manipulator = new ScheduleItemManipulator(item);
		if (manipulator) {
			if (!fDefaultState->AddManipulator(manipulator)) {
				delete manipulator;
				printf("ScheduleView::_MakeManipulators() - "
					"unable to add manipulator\n");
				break;
			}
			manipulator->SetHighlighted(item->Playlist() == fHighlightPlaylist,
				false);
		}
	}

	_RebuildProblemRanges();

	if (fLocker)
		fLocker->ReadUnlock();
}

// _InvalidateManipulators
void
ScheduleView::_InvalidateManipulators()
{
	if (fLocker && !fLocker->ReadLock())
		return;

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator = fDefaultState->ManipulatorAtFast(i);
		manipulator->RebuildCachedData();
	}

	_RebuildProblemRanges();

	if (fLocker)
		fLocker->ReadUnlock();

	Invalidate();
}

// _DrawTimeColumn
void
ScheduleView::_DrawTimeColumn(BView* view, BRect updateRect)
{
	BRect r(Bounds());
	r.right = TIME_COLUMN_WIDTH - 1;
	r = r & updateRect;
	if (!r.IsValid())
		return;

	view->PushState();

	rgb_color background = view->LowColor();
	rgb_color lighten1 = tint_color(background, B_LIGHTEN_1_TINT);
	rgb_color lighten2 = tint_color(background, B_LIGHTEN_2_TINT);

	BRect line(r);
	float center = (line.left + line.right) / 2 - 10;
	for (int32 i = 0; i < 24; i++) {
		line.top = HeightForTime(i * 60 * 60);
		line.bottom = HeightForTime((i + 1) * 60 * 60) - 1;

		rgb_color lowColor = (i & 1) == 0 ? lighten1 : lighten2;
		view->SetLowColor(lowColor);
		view->FillRect(line, B_SOLID_LOW);

		BString timeText;
		timeText << i << ":00";

		BPoint text(center - view->StringWidth(timeText.String()),
			line.top + (fTextLineHeight + fTextAscent) / 2.0);
		view->DrawString(timeText.String(), text);
	}

	if (line.bottom < r.bottom) {
		line.top = line.bottom + 1;
		line.bottom = r.bottom;
		view->SetLowColor(background);
		view->FillRect(line, B_SOLID_LOW);
	}

	BFont font;
	view->GetFont(&font);
	font.SetSize(max_c(9.0, font.Size() * 0.8));
	view->SetFont(&font, B_FONT_SIZE);
	font_height fh;
	font.GetHeight(&fh);
	float gapLabelHeight = ceilf(1.05 * ceilf(fh.ascent) + ceilf(fh.descent));

	// indicate problems
	int32 problems = fProblemRanges.CountItems();
	float lastGapBaseline = 0.0;
	for (int32 i = 0; i < problems; i++) {
		problem_range* problem
			= (problem_range*)fProblemRanges.ItemAtFast(i);
		BRect problemArea(r);
		problemArea.top = HeightForTime(problem->startFrame / 25);
		problemArea.bottom = HeightForTime(problem->endFrame / 25);
		if (problemArea.IsValid()) {
			if (problem->gap)
				view->SetHighColor(90, 90, 90);
			else
				view->SetHighColor(255, 0, 0);
			view->FillRect(problemArea);
			if (problem->gap && lastGapBaseline < problemArea.bottom) {
				view->SetHighColor(50, 50, 50);
				lastGapBaseline = problemArea.bottom + gapLabelHeight;

				uint64 gapDuration = problem->endFrame
					- problem->startFrame + 1;
				BString durationString = string_for_frame(gapDuration);

				BPoint textPoint;
				textPoint.y = lastGapBaseline;
				textPoint.x = problemArea.right - 2.0 - view->StringWidth(
					durationString.String());
				view->DrawString(durationString.String(), textPoint);
			}
		}
	}

	view->PopState();
}

// _DrawCenteredMessage
void
ScheduleView::_DrawCenteredMessage(BView* view, BRect r, const char* message)
{
	BPoint textPos;
	textPos.x = r.left + floorf((r.Width() - StringWidth(message)) / 2);
	view->SetFont(be_bold_font);
	font_height fh;
	view->GetFontHeight(&fh);
	textPos.y = r.top + floorf((r.Height() + fh.ascent) / 2);
	view->SetHighColor(50, 50, 50);
	view->DrawString(message, textPos);
}

// _ScheduleRect
BRect
ScheduleView::_ScheduleRect() const
{
	AutoReadLocker _(fLocker);

	BRect r;
	r.left = 0;
	r.top = 0;
	r.right = TIME_COLUMN_WIDTH + SCHEDULE_COLUMN_WIDTH;
	r.bottom = HeightForTime(24 * 60 * 60) - 1;
	if (fSchedule && fSchedule->CountItems() > 0) {
		ScheduleItem* item = fSchedule->ItemAt(fSchedule->CountItems() - 1);
		r.bottom = max_c(r.bottom, HeightForTime(
			(uint32)((item->StartFrame() + item->Duration()) / 25)) + 20);
	}
	r = r | Bounds().OffsetToCopy(B_ORIGIN);
	return r;
}

// _GetSelectedItems
ScheduleItem**
ScheduleView::_GetSelectedItems(int32& count) const
{
	// NOTE: document needs to be locked

	BList itemList;
	count = fSelection->CountSelected();
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* item
			= dynamic_cast<ScheduleItem*>(fSelection->SelectableAtFast(i));
		if (item)
			itemList.AddItem((void*)item);
	}

	count = itemList.CountItems();
	if (count <= 0)
		return NULL;

	ScheduleItem** items = new (nothrow) ScheduleItem*[count];
	if (!items)
		return NULL;

	memcpy(items, itemList.Items(), count * sizeof(ScheduleItem*));

	return items;
}

// #pragma mark -

// _ClearProblemRanges
void
ScheduleView::_ClearProblemRanges()
{
	int32 count = fProblemRanges.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (problem_range*)fProblemRanges.ItemAtFast(i);
	fProblemRanges.MakeEmpty();
}

// _RebuildProblemRanges
void
ScheduleView::_RebuildProblemRanges()
{
	_ClearProblemRanges();

	if (!fSchedule)
		return;

	int32 count = fSchedule->CountItems();
	uint64 startFrame = 0;
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* item = fSchedule->ItemAtFast(i);
		uint64 itemStartFrame = item->StartFrame();
		uint64 nextStartFrame = itemStartFrame + item->Duration();
		problem_range* problem = NULL;
		if (itemStartFrame > startFrame) {
			problem = new (nothrow) problem_range(startFrame, itemStartFrame,
				true);
		} else if (itemStartFrame < startFrame) {
			problem = new (nothrow) problem_range(itemStartFrame, startFrame,
				false);
		}
		if (problem && !fProblemRanges.AddItem(problem))
			delete problem;
		// TODO: could combine problem ranges here (only assign if
		// nextStartFrame > startFrame)
		startFrame = nextStartFrame;
	}

	BRect dirty(Bounds());
	dirty.right = TIME_COLUMN_WIDTH - 1;
	Invalidate(dirty);
}



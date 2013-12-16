/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TimelineView.h"

#include <new>
#include <stdio.h>

#include <Autolock.h>
#include <Clipboard.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Region.h>
#include <ScrollBar.h>
#include <TextView.h>
#include <Window.h>

#include "common.h"
#include "support.h"
#include "support_ui.h"

#include "CloseGapCommand.h"
#include "CompoundCommand.h"
#include "CurrentFrame.h"
#include "CutCommand.h"
#include "DeleteCommand.h"
#include "DisplayRange.h"
#include "DuplicateItemsCommand.h"
#include "EditorVideoView.h"
#include "InsertClipsDropState.h"
#include "InsertCommand.h"
#include "LoopMode.h"
#include "MultipleManipulatorState.h"
#include "PlaybackManager.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "PlaylistItemManipulator.h"
#include "ReplaceClipDropState.h"
#include "RWLocker.h"
#include "Scroller.h"
#include "Selectable.h"
#include "Selection.h"
#include "SetAudioMutedCommand.h"
#include "SetVideoMutedCommand.h"
#include "TimelineMessages.h"
#include "TimelineTool.h"
#include "TimeView.h"
#include "TrackView.h"

using std::nothrow;

enum {
	MSG_CLOSE_GAP		= 'clgp'
};

// constructor
TimelineView::TimelineView(const char* name)
	: StateView(name, B_WILL_DRAW)
	, fPlaylist(NULL)
	, fClipLibrary(NULL)

	, fPlaybackManager(NULL)
	, fCurrentFrame(NULL)
	, fCurrentFrameMarker(0)

	, fDisplayRange(NULL)
	, fLoopMode(NULL)
	, fZoomLevel(1.0)
	, fZooming(false)
	, fDisplayedFrames(0)
	, fAutoscroll(true)
	, fAutoscrollFollowMode(FOLLOW_MODE_PAGE)

	, fSelection(NULL)
	, fClipboard(be_clipboard)

	, fTool(NULL)
	, fDefaultState(new MultipleManipulatorState(this))
	, fInsertDropAnticipationState(new InsertClipsDropState(this))
	, fReplaceDropAnticipationState(new ReplaceClipDropState(this))

	, fTimeView(NULL)
	, fTrackView(NULL)
{
}

// destructor
TimelineView::~TimelineView()
{
	_DeleteManipulators();

	delete fDefaultState;
	delete fInsertDropAnticipationState;
	delete fReplaceDropAnticipationState;

	_DeleteManipulators();

	if (fDisplayRange)
		fDisplayRange->RemoveObserver(this);

	if (fCurrentFrame)
		fCurrentFrame->RemoveObserver(this);

	if (fPlaylist)
		fPlaylist->RemoveListObserver(this);
}

// #pragma mark -

// AttachedToWindow
void
TimelineView::AttachedToWindow()
{
	StateView::AttachedToWindow();
	SetLowColor(180, 180, 180, 255);
	SetViewColor(B_TRANSPARENT_COLOR);

	SetState(fDefaultState);
	MakeFocus();

	SetDataRect(_TimelineRect());
}

// DetachedFromWindow
void
TimelineView::DetachedFromWindow()
{
	SetState(NULL);

	StateView::DetachedFromWindow();
}

// MessageReceived
void
TimelineView::MessageReceived(BMessage* message)
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
			PlaylistItem* item;
			if (message->FindPointer("item", (void**)&item) == B_OK)
				RemoveItem(item);
			break;
		}
		case MSG_SET_VIDEO_MUTED: {
			PlaylistItem* item;
			if (message->FindPointer("item", (void**)&item) == B_OK)
				PerformCommand(new (nothrow) SetVideoMutedCommand(item));
			break;
		}
		case MSG_SET_AUDIO_MUTED: {
			PlaylistItem* item;
			if (message->FindPointer("item", (void**)&item) == B_OK)
				PerformCommand(new (nothrow) SetAudioMutedCommand(item));
			break;
		}

		case MSG_CLOSE_GAP: {
			uint32 track;
			int64 gapStartFrame;
			int64 gapEndFrame;
			if (message->FindInt32("track", (int32*)&track) == B_OK
				&& message->FindInt64("gap start", &gapStartFrame) == B_OK
				&& message->FindInt64("gap end", &gapEndFrame) == B_OK) {
				PerformCommand(new (nothrow) CloseGapCommand(fPlaylist, track,
					gapStartFrame, gapEndFrame));
			}
			break;
		}

		case MSG_CUT_ITEMS:
			CutSelectedItems();
			break;
		case MSG_REMOVE_ITEMS:
			RemoveSelectedItems();
			break;
		case MSG_DUPLICATE_ITEMS:
			DuplicateSelectedItems();
			break;

		case MSG_ZOOM_OUT:
			_SetZoom(_NetZoomOutLevel(fZoomLevel));
			break;
		case MSG_ZOOM_IN:
			_SetZoom(_NetZoomInLevel(fZoomLevel));
			break;

		default:
			StateView::MessageReceived(message);
			break;
	}
}

// MouseDown
void
TimelineView::MouseDown(BPoint where)
{
	MakeFocus(true);
	StateView::MouseDown(where);

	SetAutoscrollEnabled(false);
}

// MouseUp
void
TimelineView::MouseUp(BPoint where)
{
	StateView::MouseUp(where);

	SetAutoscrollEnabled(true);
}

// Draw
void
TimelineView::Draw(BRect updateRect)
{
	SetDrawingMode(B_OP_COPY);
	FillRect(updateRect, B_SOLID_LOW);

	StateView::Draw(updateRect);

	// current frame marker
	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
	SetHighColor(0, 200, 0, 120);
	BRect r = _CurrentFrameMarkerRect();
	StrokeLine(r.LeftTop(), r.LeftBottom());
}

// NothingClicked
void
TimelineView::NothingClicked(BPoint where, uint32 buttons, uint32 clicks)
{
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		_ShowPopupMenuForEmptiness(where);
		return;
	}

	if (fSelection)
		fSelection->DeselectAll();
}

// MouseWheelChanged
bool
TimelineView::MouseWheelChanged(BPoint where, float x, float y)
{
	if (!Bounds().Contains(where))
		return false;

	if (y > 0)
		_SetZoom(_NetZoomOutLevel(fZoomLevel));
	else if (y < 0)
		_SetZoom(_NetZoomInLevel(fZoomLevel));

	return true;
}

// #pragma mark -

// _HandleKeyDown
bool
TimelineView::_HandleKeyDown(const KeyEvent& event, BHandler* originalTarget)
{
	if (dynamic_cast<BTextView*>(originalTarget))
		return false;

	EditorVideoView* stage = dynamic_cast<EditorVideoView*>(originalTarget);
	if (stage && stage->HandlesAllKeyEvents())
		return false;


	bool isFocus = IsFocus();
	bool handled = true;

	switch (event.key) {
		case 'x':
			if (isFocus)
				CutSelectedItems();
			else
				handled = false;
			break;

		case B_DELETE:
			if (isFocus)
				RemoveSelectedItems();
			else
				handled = false;
			break;

		// controll playback
		case B_SPACE:
			if (fPlaybackManager->Lock()) {
				fPlaybackManager->TogglePlaying();
				fPlaybackManager->Unlock();
			}
			break;

		case B_HOME: {
			if (fPlaybackManager->Lock()) {
				int64 seekFrame = fPlaybackManager->FirstPlaybackRangeFrame();
				fPlaybackManager->SetCurrentFrame(seekFrame);
				fPlaybackManager->Unlock();
			}
			break;
		}
		case B_END: {
			if (fPlaybackManager->Lock()) {
				int64 seekFrame = fPlaybackManager->LastPlaybackRangeFrame();
				fPlaybackManager->SetCurrentFrame(seekFrame);
				fPlaybackManager->Unlock();
			}
			break;
		}
		default:
			handled = false;
			break;
	}

	if (handled)
		return true;

	if (stage)
		return false;

	switch (event.key) {
		case B_UP_ARROW: {
			SnapFrameList snapFrames;
			snapFrames.CollectSnapFrames(fPlaylist, 0);
			if (fPlaybackManager->Lock()) {
				int64 seekFrame = snapFrames.ClosestSnapFrameBackwardsFor(
					fPlaybackManager->CurrentFrame() - 1);
				fPlaybackManager->SetCurrentFrame(seekFrame);
				fPlaybackManager->Unlock();
			}
			break;
		}
		case B_DOWN_ARROW: {
			SnapFrameList snapFrames;
			snapFrames.CollectSnapFrames(fPlaylist, 0);
			if (fPlaybackManager->Lock()) {
				int64 seekFrame = snapFrames.ClosestSnapFrameForwardFor(
					fPlaybackManager->CurrentFrame() + 1);
				fPlaybackManager->SetCurrentFrame(seekFrame);
				fPlaybackManager->Unlock();
			}
			break;
		}
		case B_LEFT_ARROW:
			if (fPlaybackManager->LockWithTimeout(50000) == B_OK) {
				fPlaybackManager->PausePlaying();
				fPlaybackManager->SetCurrentFrame(
					fPlaybackManager->CurrentFrame() - 1);
				fPlaybackManager->Unlock();
			}
			break;
		case B_RIGHT_ARROW:
			if (fPlaybackManager->LockWithTimeout(50000) == B_OK) {
				fPlaybackManager->PausePlaying();
				fPlaybackManager->SetCurrentFrame(
					fPlaybackManager->CurrentFrame() + 1);
				fPlaybackManager->Unlock();
			}
			break;

		case '+':
			_SetZoom(_NetZoomInLevel(fZoomLevel));
			break;

		case '-':
			_SetZoom(_NetZoomOutLevel(fZoomLevel));
			break;

		default:
			handled = false;
	}
	return handled;
}

// _HandleKeyUp
bool
TimelineView::_HandleKeyUp(const KeyEvent& event, BHandler* originalTarget)
{
	if (dynamic_cast<BTextView*>(originalTarget))
		return false;

	bool isFocus = IsFocus();
	bool handled = true;

	switch (event.key) {
		case B_DELETE:
			handled = isFocus;
			break;

		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			break;

		default:
			handled = false;
	}

	return handled;
}

// #pragma mark -

// ObjectChanged
void
TimelineView::ObjectChanged(const Observable* object)
{
	if (object == fCurrentFrame) {

		// move current frame marker
		Invalidate(_CurrentFrameMarkerRect());
		fCurrentFrameMarker = fCurrentFrame->VirtualFrame();
		Invalidate(_CurrentFrameMarkerRect());

		if (!fCurrentFrame->BeingDragged())
			_FollowPlaybackFrame();

	} else if (object == fDisplayRange) {

		if (fDisplayedFrames != fDisplayRange->DisplayedFrames()) {
			// update scrollable area
			fDisplayedFrames = fDisplayRange->DisplayedFrames();
			SetDataRect(_TimelineRect());
		}

		if (fTimeView)
			fTimeView->ObjectChanged(fDisplayRange);

		// update scroll position if necessary
		BPoint scrollOffset = ScrollOffset();
		float wantOffset = PosForFrame(fDisplayRange->FirstFrame());
		if (scrollOffset.x != wantOffset) {
			scrollOffset.x = wantOffset;
			SetScrollOffset(scrollOffset);
		}

	} else {
		debugger("TimelineView::ObjectChanged() - received "
				 "notification for unkown object!\n");
	}
}

// #pragma mark -

// ItemAdded
void
TimelineView::ItemAdded(::Playlist* playlist, PlaylistItem* item, int32 index)
{
	if (!fTool)
		debugger("TimelineView::ItemAdded() - no tool\n");

	if (!LockLooper())
		return;

	PlaylistItemManipulator* manipulator = fTool->ManipulatorFor(item);
	if (manipulator) {
		if (!fDefaultState->AddManipulator(manipulator)) {
			delete manipulator;
			print_error("TimelineView::_MakeManipulators() - "
				"unable to add manipulator\n");
		} else {
			Invalidate(manipulator->Bounds());
		}
	}

	UnlockLooper();
}

// ItemRemoved
void
TimelineView::ItemRemoved(::Playlist* playlist, PlaylistItem* item)
{
	if (!LockLooper())
		return;

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		PlaylistItemManipulator* manipulator =
			(PlaylistItemManipulator*)fDefaultState->ManipulatorAtFast(i);
		if (manipulator->Item() == item
			&& fDefaultState->RemoveManipulator(i) == manipulator) {
			Invalidate(manipulator->Bounds());
			delete manipulator;
			break;
		}
	}

	UnlockLooper();
}

// DurationChanged
void
TimelineView::DurationChanged(::Playlist* playlist, uint64 duration)
{
	if (!LockLooper())
		return;

	// update scrollable area
	SetDataRect(_TimelineRect());

	UnlockLooper();
}

// MaxTrackChanged
void
TimelineView::MaxTrackChanged(::Playlist* playlist, uint32 maxTrack)
{
	if (!LockLooper())
		return;

	// update scrollable area
	SetDataRect(_TimelineRect());

	UnlockLooper();
}

// #pragma mark -

// DataRectChanged
void
TimelineView::DataRectChanged(BRect oldDataRect, BRect newDataRect)
{
	if (!fDisplayRange)
		return;

	// make sure the first frame of the display range is not out of
	// our data rect
	int64 minFrame = min_c(FrameForPos(newDataRect.left),
						   FrameForPos(Bounds().left));
	int64 maxFrame = max_c(FrameForPos(newDataRect.right),
						   FrameForPos(Bounds().right));
	if ((fDisplayRange->FirstFrame() >= minFrame
		&& fDisplayRange->LastFrame() <= maxFrame)
		|| fZooming) {
		return;
	}
//printf("TimelineView::DataRectChanged()\n");
//printf("min: %lld/%lld, max: %lld/%lld\n", minFrame, fDisplayRange->FirstFrame(),
//		maxFrame, fDisplayRange->LastFrame());

	int64 frameOffset = FrameForPos(Bounds().left)
						- fDisplayRange->FirstFrame();
//printf("offset: %lld\n", frameOffset);
	fDisplayRange->MoveBy(frameOffset);
}

// ScrollOffsetChanged
void
TimelineView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	if (!fDisplayRange)
		return;

	// Bounds().left == 0 is mapped to fDisplayRange->FirstFrame() == 0
	// FrameForPos() will give us the mapping to frames according to this
	// assumption
	int64 frameOffset = FrameForPos(newOffset.x)
						- fDisplayRange->FirstFrame();

	fDisplayRange->SuspendNotifications(true);

	fDisplayRange->MoveBy(frameOffset);

	BPoint offset = newOffset - oldOffset;
	ScrollBy(offset.x, offset.y);

	if (fTimeView) {
		// adjust time view scrolling offset
		fTimeView->ScrollBy(offset.x, 0);
	}
	if (fTrackView) {
		// adjust track view scrolling offset
		fTrackView->ScrollBy(0, offset.y);
	}

	fDisplayRange->SuspendNotifications(false);
}

// VisibleSizeChanged
void
TimelineView::VisibleSizeChanged(float oldWidth, float oldHeight,
								 float newWidth, float newHeight)
{
	if (fDisplayRange) {
		int64 lastFrame = FrameForPos(Bounds().right);
		fDisplayRange->SetLastFrame(lastFrame);
	}
}

// #pragma mark -

// StateForDragMessage
ViewState*
TimelineView::StateForDragMessage(const BMessage* dragMessage)
{
	if (modifiers() & B_COMMAND_KEY) {
		if (fReplaceDropAnticipationState->WouldAcceptDragMessage(dragMessage))
			return fReplaceDropAnticipationState;
	}
	return fInsertDropAnticipationState;
}

// #pragma mark -

// SetPlaylist
void
TimelineView::SetPlaylist(::Playlist* list)
{
	if (fPlaylist != list) {
		uint64 duration = 0;
		if (fPlaylist)
			fPlaylist->RemoveListObserver(this);

		fPlaylist = list;

		if (fPlaylist) {
			fPlaylist->AddListObserver(this);
			duration = fPlaylist->Duration();
		}

		_MakeManipulators();
		DurationChanged(fPlaylist, duration);
	}
}

// SetClipLibrary
void
TimelineView::SetClipLibrary(ServerObjectManager* library)
{
	fClipLibrary = library;
}

// SetCurrentFrame
void
TimelineView::SetCurrentFrame(::CurrentFrame* frame)
{
	if (fCurrentFrame != frame) {
		if (fCurrentFrame)
			fCurrentFrame->RemoveObserver(this);

		fCurrentFrame = frame;

		if (fCurrentFrame) {
			fCurrentFrame->AddObserver(this);
			// trigger invalidation of
			// current frame marker
			ObjectChanged(fCurrentFrame);
		}
	}
}

// SetDisplayRange
void
TimelineView::SetDisplayRange(DisplayRange* range)
{
	if (fDisplayRange != range) {
		if (fDisplayRange)
			fDisplayRange->RemoveObserver(this);

		fDisplayRange = range;

		if (fDisplayRange) {
			fDisplayRange->AddObserver(this);
			fDisplayedFrames = fDisplayRange->DisplayedFrames();
		} else {
			fDisplayedFrames = 0;
		}

		// update DisplayRange
		float width = Bounds().Width();
		float height = Bounds().Height();
		VisibleSizeChanged(width, height, width, height);
	}
}

// SetPlaybackManager
void
TimelineView::SetPlaybackManager(::PlaybackManager* manager)
{
	fPlaybackManager = manager;
}

// SetLoopMode
void
TimelineView::SetLoopMode(LoopMode* mode)
{
	fLoopMode = mode;
}

// SetTimeView
void
TimelineView::SetTimeView(TimeView* view)
{
	fTimeView = view;

	if (fTimeView) {
		// immidiately adjust the view's scroll offset to our's
		fTimeView->ScrollTo(Bounds().left, 0);
	}
}

// SetTrackView
void
TimelineView::SetTrackView(TrackView* view)
{
	fTrackView = view;

	if (fTrackView) {
		// immidiately adjust the view's scroll offset to our's
		fTrackView->ScrollTo(0, Bounds().top);
	}
}

// SetAutoscrollEnabled
void
TimelineView::SetAutoscrollEnabled(bool enabled)
{
	fAutoscroll = enabled;
}

// SetAutoscrollFollowMode
void
TimelineView::SetAutoscrollFollowMode(uint32 mode, bool followNow)
{
	if (fAutoscrollFollowMode == mode)
		return;

	fAutoscrollFollowMode = mode;

	if (followNow)
		_FollowPlaybackFrame();
}

// #pragma mark -

// SetSelection
void
TimelineView::SetSelection(::Selection* selection)
{
	fSelection = selection;
}

// SetTool
void
TimelineView::SetTool(TimelineTool* tool)
{
	if (tool != fTool) {
		fTool = tool;
		// get new manipulators from the tool
		_MakeManipulators();
	}
}

// Select
void
TimelineView::Select(Selectable* object)
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

// #pragma mark -

// CopySelectedItems
void
TimelineView::CopySelectedItems()
{
	int32 count;
	PlaylistItem** items = _GetSelectedItems(count);

	if (count <= 0) {
		printf("no item selected\n");
		return;
	}

	BMessage itemsArchive;
	for (int32 i = 0; i < count; i++) {
		BMessage archive;
		if (items[i]->Archive(&archive, true) < B_OK) {
			printf("error archiving item %ld\n", i);
			return;
		}

		if (itemsArchive.AddMessage("item", &archive) < B_OK) {
			printf("error adding archive %ld to clipboard\n", i);
			return;
		}
	}

	if (!fClipboard->Lock()) {
		printf("error locking clipboard\n");
		return;
	}

	if (fClipboard->Clear() == B_OK) {

		BMessage* data = fClipboard->Data();
		data->AddMessage("clockwerk:playlist_items", &itemsArchive);

		fClipboard->Commit();
	} else {
		printf("error clearing clipboard\n");
	}

	fClipboard->Unlock();
}

// RemoveItem
void
TimelineView::RemoveItem(PlaylistItem* item)
{
	if (!item || !fPlaylist || !fSelection)
		return;

	AutoWriteLocker locker(Locker());
	if (Locker() && !locker.IsLocked())
		return;

	PlaylistItem** items = new (nothrow) PlaylistItem*[1];
	if (!items)
		return;

	items[0] = item;
	DeleteCommand* command = new (nothrow) DeleteCommand(
		fPlaylist, items, 1, fSelection);

	PerformCommand(command);
}

// CutSelectedItems
void
TimelineView::CutSelectedItems()
{
	if (!fPlaylist || !fSelection)
		return;

	AutoWriteLocker locker(Locker());
	if (Locker() && !locker.IsLocked())
		return;

	int32 count;
	PlaylistItem** items = _GetSelectedItems(count);
	if (items == NULL || count == 0)
		return;

	Command** commands = new (nothrow) Command*[count];
	if (commands == NULL) {
		delete[] items;
		return;
	}

	for (int32 i = 0; i < count; i++) {
		commands[i] = new (nothrow) CutCommand(items[i],
			fCurrentFrame->Frame());
	}

	delete[] items;

	CompoundCommand* compoundCommand = new (nothrow) CompoundCommand(
		commands, count, count > 1 ? "Cut items" : "Cut item", 0);
	if (compoundCommand == NULL) {
		for (int32 i = 0; i < count; i++)
			delete commands[i];
		delete[] commands;
		return;
	}

	PerformCommand(compoundCommand);
}

// RemoveSelectedItems
void
TimelineView::RemoveSelectedItems()
{
	if (!fPlaylist || !fSelection)
		return;

	AutoWriteLocker locker(Locker());
	if (Locker() && !locker.IsLocked())
		return;

	int32 count;
	PlaylistItem** items = _GetSelectedItems(count);

	DeleteCommand* command = new (nothrow) DeleteCommand(
		fPlaylist, items, count, fSelection);

	PerformCommand(command);
}

// DuplicateSelectedItems
void
TimelineView::DuplicateSelectedItems()
{
	if (!fPlaylist || !fSelection)
		return;

	AutoWriteLocker locker(Locker());
	if (Locker() && !locker.IsLocked())
		return;

	int32 count;
	PlaylistItem** items = _GetSelectedItems(count);

	DuplicateItemsCommand* command = new (nothrow) DuplicateItemsCommand(
		fPlaylist, (const PlaylistItem**)items, count, fSelection);

	delete[] items;

	PerformCommand(command);
}

// Paste
void
TimelineView::Paste()
{
	if (!fPlaylist || !fClipLibrary || !fClipboard->Lock())
		return;

	BMessage itemsArchive;
	BMessage* data = fClipboard->Data();
	data->FindMessage("clockwerk:playlist_items", &itemsArchive);

	fClipboard->Unlock();

	if (itemsArchive.IsEmpty())
		return;

	BList items;

	BMessage archive;
	for (int32 i = 0; itemsArchive.FindMessage("item", i, &archive) == B_OK;
		 i++) {
		BArchivable* archivable = instantiate_object(&archive);
		PlaylistItem* item = dynamic_cast<PlaylistItem*>(archivable);
		if (item) {
			item->ResolveDependencies(fClipLibrary);
			if (!items.AddItem((void*)item))
				delete item;
		} else {
			printf("failed to instantiate PlaylistItem from archive %ld\n", i);
		}
	}

	// TODO: ...
	PerformCommand(new InsertCommand(fPlaylist, fSelection,
		(PlaylistItem**)items.Items(), items.CountItems(), fCurrentFrameMarker,
		fPlaylist->MaxTrack() + 1));
}

// #pragma mark -

// TrackHeight
uint32
TimelineView::TrackHeight() const
{
	return 48;
}

// StartFrame
int64
TimelineView::StartFrame() const
{
	if (fDisplayRange)
		return fDisplayRange->FirstFrame();
	return FrameForPos(Bounds().left);
}

// FramesPerPixel
double
TimelineView::FramesPerPixel() const
{
	return fZoomLevel;
}

// FrameForPos
int64
TimelineView::FrameForPos(float x) const
{
	return (int64)(roundf(x * FramesPerPixel()));
}

// PosForFrame
float
TimelineView::PosForFrame(int64 frame) const
{
	return roundf(frame / FramesPerPixel());
}

// TrackForPos
uint32
TimelineView::TrackForPos(float y) const
{
	if (y < 0.0)
		return 0;
	return uint32((uint32)y / TrackHeight());
}

// IsPaused
bool
TimelineView::IsPaused() const
{
	BAutolock _(fPlaybackManager);
	return !fPlaybackManager->IsPlaying();
}

// CurrentFrame
int64
TimelineView::CurrentFrame() const
{
	return fCurrentFrameMarker;
}


// #pragma mark -

// _DeleteManipulators
void
TimelineView::_DeleteManipulators()
{
	if (LockLooper()) {
		fDefaultState->DeleteManipulators();
		UnlockLooper();
	}
}

// _MakeManipulators
void
TimelineView::_MakeManipulators()
{
	_DeleteManipulators();

	Invalidate();

	if (!fPlaylist | !fTool)
		return;

	if (fLocker && !fLocker->ReadLock())
		return;

	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = fPlaylist->ItemAtFast(i);
		PlaylistItemManipulator* manipulator = fTool->ManipulatorFor(item);
		if (manipulator) {
			if (!fDefaultState->AddManipulator(manipulator)) {
				delete manipulator;
				print_error("TimelineView::_MakeManipulators() - "
					"unable to add manipulator\n");
				break;
			}
		}
	}

	if (fLocker)
		fLocker->ReadUnlock();
}

// _InvalidateManipulators
void
TimelineView::_InvalidateManipulators()
{
	if (fLocker && !fLocker->ReadLock())
		return;

	int32 count = fDefaultState->CountManipulators();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator = fDefaultState->ManipulatorAtFast(i);
		manipulator->RebuildCachedData();
	}

	if (fLocker)
		fLocker->ReadUnlock();

	Invalidate();
}

// _CurrentFrameMarkerRect
BRect
TimelineView::_CurrentFrameMarkerRect() const
{
	BRect r(Bounds());
	r.left = r.right = PosForFrame(fCurrentFrameMarker);
	return r;
}

// _TimelineRect
BRect
TimelineView::_TimelineRect() const
{
	BRect r(0, 0, -1, -1);
	if (fPlaylist) {
		int64 firstFrame;
		int64 lastFrame;
		fPlaylist->GetFrameBounds(&firstFrame, &lastFrame);

		r.left = PosForFrame(firstFrame) - 100.0;
		r.top = 0.0;
		r.right = PosForFrame(lastFrame) + 100.0;
		r.bottom = TrackHeight() * (fPlaylist->MaxTrack() + 2);
	}
	r = r | Bounds().OffsetToCopy(B_ORIGIN);
	return r;
}

// _GetSelectedItems
PlaylistItem**
TimelineView::_GetSelectedItems(int32& count) const
{
	// NOTE: document needs to be locked

	BList itemList;
	count = fSelection->CountSelected();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item
			= dynamic_cast<PlaylistItem*>(fSelection->SelectableAtFast(i));
		if (item)
			itemList.AddItem((void*)item);
	}

	count = itemList.CountItems();
	if (count <= 0)
		return NULL;

	PlaylistItem** items = new (nothrow) PlaylistItem*[count];
	if (!items)
		return NULL;

	memcpy(items, itemList.Items(), count * sizeof(PlaylistItem*));

	return items;
}

// #pragma mark -

// _NetZoomOutLevel
double
TimelineView::_NetZoomOutLevel(double zoom) const
{
	if (zoom < 0.125)
		return 0.125;
	if (zoom < 0.25)
		return 0.25;
	if (zoom < 0.50)
		return 0.50;
	if (zoom < 1)
		return 1;
	if (zoom < 2)
		return 2;
	if (zoom < 4)
		return 4;
	if (zoom < 8)
		return 8;
	if (zoom < 16)
		return 16;
	if (zoom < 32)
		return 32;
	if (zoom < 64)
		return 64;
	return 128;
}

// _NetZoomInLevel
double
TimelineView::_NetZoomInLevel(double zoom) const
{
	if (zoom > 64)
		return 64;
	if (zoom > 32)
		return 32;
	if (zoom > 16)
		return 16;
	if (zoom > 8)
		return 8;
	if (zoom > 4)
		return 4;
	if (zoom > 2)
		return 2;
	if (zoom > 1)
		return 1;
	if (zoom > 0.5)
		return 0.5;
	if (zoom > 0.25)
		return 0.25;
	if (zoom > 0.125)
		return 0.125;
	return 0.075;
}

// _SetZoom
void
TimelineView::_SetZoom(float zoomLevel)
{
	if (fZoomLevel == zoomLevel)
		return;

	double displayPixels = Bounds().Width();
	int64 oldDisplayedFrames = (int64)ceil(displayPixels * fZoomLevel);

	fZoomLevel = zoomLevel;

	int64 firstFrame = fDisplayRange->FirstFrame();
	float playbackPosFactor = (float)(fCurrentFrameMarker - firstFrame)
		/ (float)oldDisplayedFrames;

	int64 newDisplayedFrames = (int64)ceil(displayPixels * fZoomLevel);

	int64 shift;
	if (playbackPosFactor >= 0.0 && playbackPosFactor <= 1.0) {
		shift = (int64)((oldDisplayedFrames - newDisplayedFrames)
			* playbackPosFactor);
	} else {
		int64 oldCenter = firstFrame + oldDisplayedFrames / 2;
		int64 newCenter = firstFrame + newDisplayedFrames / 2;
		shift = oldCenter - newCenter;
	}

	fZooming = true;
	fDisplayRange->SuspendNotifications(true);

	// zoom
	fDisplayRange->SetLastFrame(fDisplayRange->FirstFrame()
		+ newDisplayedFrames);
	// zoom into middle
	fDisplayRange->MoveBy(shift);

	fDisplayRange->SuspendNotifications(false);
	fZooming = false;

	_InvalidateManipulators();
}

// _FollowPlaybackFrame
void
TimelineView::_FollowPlaybackFrame()
{
	if (fLoopMode && fLoopMode->Mode() == LOOPING_VISIBLE)
		return;

	// HACK: work arround BScrollBars not receiving B_MOUSE_DOWN/UP:
	// (We cannot override BScrollBar::MouseDown() to know when a scroll
	// bar is being grabbed with the mouse, we simply check wether a
	// button is pressed and bail out if so.)
	BPoint mousePos;
	uint32 buttons;
	GetMouse(&mousePos, &buttons, false);
	if (buttons != 0)
		return;

	if (!fDisplayRange || !fAutoscroll
		|| fAutoscrollFollowMode == FOLLOW_MODE_OFF)
		return;

	int64 room = int64(50 * fZoomLevel);

	int64 minFrame = fDisplayRange->FirstFrame() + room;
	int64 maxFrame = fDisplayRange->LastFrame() - room;
	if (fCurrentFrameMarker > minFrame && fCurrentFrameMarker < maxFrame)
		return;

	int64 wantFirstFrame = fDisplayRange->FirstFrame();

	switch (fAutoscrollFollowMode) {
		case FOLLOW_MODE_PAGE: {
			wantFirstFrame = fCurrentFrameMarker - room;
			int64 offset = wantFirstFrame - fDisplayRange->FirstFrame();
			// animate scroll
			int64 offsetPart1 = (int64)(offset * 0.07);
			int64 offsetPart2 = (int64)(offset * 0.15);
			int64 offsetPart3 = (int64)((offset - 2 * (offsetPart1 + offsetPart2)) * 0.5);
			int64 offsetPart4 = offset - offsetPart1 - 2 * (offsetPart2 + offsetPart3);
			fDisplayRange->MoveBy(offsetPart1);
			Window()->UpdateIfNeeded();
			fDisplayRange->MoveBy(offsetPart2);
			Window()->UpdateIfNeeded();
			fDisplayRange->MoveBy(offsetPart3);
			Window()->UpdateIfNeeded();
			fDisplayRange->MoveBy(offsetPart3);
			Window()->UpdateIfNeeded();
			fDisplayRange->MoveBy(offsetPart2);
			Window()->UpdateIfNeeded();
			fDisplayRange->MoveBy(offsetPart4);
			break;
		}
		case FOLLOW_MODE_INCREMENTAL:
			// TODO: implement
			break;
	}
}

// _ShowPopupMenuForEmptiness
void
TimelineView::_ShowPopupMenuForEmptiness(BPoint where)
{
	if (!fPlaylist)
		return;

	BPopUpMenu* menu = new BPopUpMenu("item popup", false, false);

	// find an item after this position
	int64 frame = FrameForPos(where.x);
	uint32 track = TrackForPos(where.y);
	int64 gapStartFrame = -LONG_MAX;
	int64 gapEndFrame = LONG_MAX;
	int32 count = fPlaylist->CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaylistItem* item = fPlaylist->ItemAtFast(i);
		if (item->Track() == track && item->EndFrame() < frame) {
			if (item->EndFrame() > gapStartFrame)
				gapStartFrame = item->EndFrame();
		}
		if (item->Track() == track && item->StartFrame() > frame) {
			if (item->StartFrame() <= gapEndFrame)
				gapEndFrame = item->StartFrame() - 1;
		}
	}

	BMessage* message;
	BMenuItem* item;

	if (gapStartFrame > -LONG_MAX && gapEndFrame < LONG_MAX) {
		message = new BMessage(MSG_CLOSE_GAP);
		message->AddInt32("track", track);
		message->AddInt64("gap start", gapStartFrame);
		message->AddInt64("gap end", gapEndFrame);
		item = new BMenuItem("Close Gap", message);
		menu->AddItem(item);
	}

	menu->SetTargetForItems(this);

	show_popup_menu(menu, where, this, false);
}


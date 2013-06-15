/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleItemManipulator.h"

#include <new>

#include <stdio.h>

#include <Cursor.h>
#include <Looper.h>
#include <Message.h>
#include <View.h>

#include "cursors.h"
#include "support.h"
#include "support_ui.h"
#include "ui_defines.h"

#include "ChangeScheduleItemCommand.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "ScheduleView.h"

using std::nothrow;

enum {
	DRAGGING_NONE	= 0,
	DRAGGING_BOTTOM
};

enum {
	MSG_SCHEDULE_ITEM_CHANGED	= 'sich',
};

// constructor
ScheduleItemManipulator::ScheduleItemManipulator(ScheduleItem* item)
	: Manipulator(NULL)
	, fItem(item)
	, fItemFrame(0.0, 0.0, -1.0, -1.0)

	, fCachedStartFrame(0)
	, fCachedDuration(0)
	, fCachedName("")

	, fCachedSelected(false)
	, fCachedFlexibleDuration(false)

	, fPreparedToResize(false)
	, fHighlighted(false)

	, fView(NULL)

	, fDragMode(DRAGGING_NONE)
{
	if (fItem) {
		fItem->Acquire();
		fItem->AddObserver(this);
		fCachedStartFrame = fItem->StartFrame();
		fCachedDuration = fItem->Duration();
		fCachedName = fItem->Name();

		fCachedSelected = fItem->IsSelected();
		fCachedFlexibleDuration = fItem->FlexibleDuration();
	}
}

// destructor
ScheduleItemManipulator::~ScheduleItemManipulator()
{
	if (fItem) {
		fItem->RemoveObserver(this);
		fItem->Release();
	}
}

// #pragma mark -

// ObjectChanged
void
ScheduleItemManipulator::ObjectChanged(const Observable* object)
{
	// NOTE: accessing these members here from any thread is not safe
	// but the worst that could happen is that an invalidation happens
	// unnecessarily. fView member is more critical, but manipulators
	// should be destroyed and detached before fView becomes invalid.

	if (object != fItem || !fView)
		return;

	if ((fCachedStartFrame != fItem->StartFrame()
		 || fCachedDuration != fItem->Duration()
		 || fCachedSelected != fItem->IsSelected()
		 || fCachedFlexibleDuration != fItem->FlexibleDuration()
		 || fCachedName != fItem->Name())) {

		if (fView->Looper()) {
			BMessage message(MSG_SCHEDULE_ITEM_CHANGED);
			message.AddPointer("object", object);
			fView->Looper()->PostMessage(&message, fView);
		} else
			_ObjectChanged();
	}
}

// #pragma mark -


// Draw
void
ScheduleItemManipulator::Draw(BView* into, BRect updateRect)
{
	if (fItemFrame.IsValid()) {
		into->SetDrawingMode(B_OP_COPY);

		bool empty = fItem && !fItem->Playlist();
		BRect r(fItemFrame);

		if (!empty) {
			into->SetHighColor(0, 0, 0, 255);
			into->StrokeRect(r);
			if (r.IntegerHeight() < 3)
				return;
		}

		rgb_color base;
		if (!empty) {
			into->SetDrawingMode(B_OP_ALPHA);
			into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

			if (fCachedFlexibleDuration)
				base = (rgb_color){ 185, 185, 200, 200 };
			else
				base = (rgb_color){ 220, 220, 220, 200 };

			if (fHighlighted)
				base = (rgb_color){ 255, 217, 121, 200 };

			if (fCachedSelected)
				base = tint_color(base, B_DARKEN_2_TINT);

			into->SetHighColor(base);
			into->SetLowColor(base);

			rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
			rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

			r.InsetBy(1, 1);

			into->BeginLineArray(4);
				into->AddLine(BPoint(r.left, r.bottom - 1),
							  BPoint(r.left, r.top), lighten2);
				into->AddLine(BPoint(r.left + 1, r.top),
							  BPoint(r.right - 1, r.top), lighten2);
				into->AddLine(BPoint(r.right, r.top),
							  BPoint(r.right, r.bottom - 1), darken2);
				into->AddLine(BPoint(r.right, r.bottom),
							  BPoint(r.left, r.bottom), darken2);
			into->EndLineArray();

			r.InsetBy(1, 1);
			into->FillRect(r);
		} else {
			into->SetDrawingMode(B_OP_SUBTRACT);

			base = (rgb_color){ 20, 20, 20, 0 };
			if (fCachedSelected)
				base = (rgb_color){ 40, 40, 40, 0 };
			rgb_color darken2 = (rgb_color){
				base.red + 10, base.green + 10, base.blue + 10, 0 };

			into->SetHighColor(base);
			into->SetLowColor(darken2);

			into->FillRect(r, kStripes);
		}

		if (r.IntegerHeight() <= 0)
			return;

		into->SetDrawingMode(B_OP_ALPHA);
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

		// figure out text bottom line
		font_height fh;
		be_bold_font->GetHeight(&fh);
		float ascent = fh.ascent;
		be_plain_font->GetHeight(&fh);
		ascent = max_c(fh.ascent, ascent);
		float textBottom = fItemFrame.top + (fView->TextLineHeight() + ascent)
			/ 2.0 - 1.0;

		rgb_color timeColor = (rgb_color){ 0, 0, 0, 200 };
		if (fItem && !fItem->FlexibleStartFrame())
			timeColor = (rgb_color){ 150, 0, 0, 200 };

		// item time
		BString time;
		string_for_frame_of_day(time, fCachedStartFrame);

		r.InsetBy(5, 0);

		into->SetHighColor(timeColor);
		into->SetFont(be_bold_font);
		into->DrawString(time.String(), BPoint(r.left, textBottom));

		// place item name besides time unless it is truncated and
		// there is enough room below the time
		r.left += be_bold_font->StringWidth("00:00:00.00") + 5;

		// item name
		BString truncated(fCachedName);
		be_plain_font->TruncateString(&truncated, B_TRUNCATE_END, r.Width());

		if (truncated != fCachedName
			&& textBottom + fView->TextLineHeight() < r.bottom - 5) {
			// name is truncated and there is enough room below the time
			textBottom += fView->TextLineHeight();
			r.left -= be_bold_font->StringWidth("00:00:00.00") + 5;
			// truncate again for the additional room
			truncated = fCachedName;
			be_plain_font->TruncateString(&truncated, B_TRUNCATE_END,
				r.Width());
		}

		into->SetHighColor(0, 0, 0, 200);
		into->SetFont(be_plain_font);
		into->DrawString(truncated.String(), BPoint(r.left, textBottom));

		if (!fCachedFlexibleDuration)
			return;

		r = fItemFrame;
		r.InsetBy(5, 2);
		r.top = r.bottom - 2;

		rgb_color light = (rgb_color){ 255, 255, 255, empty ? 80 : 240 };
		rgb_color shadow = (rgb_color){ 0, 0, 0, empty ? 60 : 80 };

		BPoint dot = r.LeftTop();
		BPoint stop = r.RightTop();
		int32 num = 1;
		while (dot.x <= stop.x) {
			rgb_color col1;
			rgb_color col2;
			switch (num) {
				case 1:
					col1 = shadow;
					col2 = base;
					break;
				case 2:
					col1 = base;
					col2 = light;
					break;
				case 3:
					col1 = base;
					col2 = base;
					num = 0;
					break;
			}
			if (col1 != base) {
				into->SetHighColor(col1);
				into->StrokeLine(dot, dot, B_SOLID_HIGH);
			}
			dot.y++;
			if (col2 != base) {
				into->SetHighColor(col2);
				into->StrokeLine(dot, dot, B_SOLID_HIGH);
			}
			dot.y -= 1.0;
			// next pixel
			num++;
			dot.x++;
		}
	}
}

// MouseDown
bool
ScheduleItemManipulator::MouseDown(BPoint where)
{
	if (fItem && fItemFrame.Contains(where)) {
		fView->Select(fItem);

		if (fPreparedToResize) {
			fDragMode = DRAGGING_BOTTOM;
		}
		return true;
	}
	return false;
}

// MouseMoved
void
ScheduleItemManipulator::MouseMoved(BPoint where)
{
	if (!fItem || !fView)
		return;
	switch (fDragMode) {
		case DRAGGING_BOTTOM: {
			ScheduleNotificationBlock _(fView->Schedule());

			uint64 frame = fView->FrameForHeight(where.y, fItem);
			if (frame < fItem->StartFrame())
				frame = fItem->StartFrame();

			uint64 duration = frame - fItem->StartFrame();
			uint16 repeats = fItem->ExplicitRepeats();

			uint64 playlistDuration = fItem->PlaylistDuration();
			if (playlistDuration > 0) {
				repeats = (duration + playlistDuration / 2) / playlistDuration;
				duration = playlistDuration;
			}

			fView->PerformCommand(new (nothrow) ChangeScheduleItemCommand(
				fView->Schedule(), fView->Selection(), fItem,
				fItem->StartFrame(), duration, repeats,
				fItem->FlexibleStartFrame(), fItem->FlexibleDuration()));
			break;
		}
	}
}

// MouseUp
Command*
ScheduleItemManipulator::MouseUp()
{
	fDragMode = DRAGGING_NONE;
//	Command* command = ToolMouseUp();
	fView->Invalidate(fItemFrame);
//	return command;
	return NULL;
}

// MouseOver
bool
ScheduleItemManipulator::MouseOver(BPoint where)
{
	if (fItemFrame.Contains(where)) {
		BRect r(fItemFrame);
		r. top = r.bottom - 8;
		fPreparedToResize = r.Contains(where) && fCachedFlexibleDuration;
		return true;
	}
	return false;
}

// DoubleClicked
bool
ScheduleItemManipulator::DoubleClicked(BPoint where)
{
	return false;
}

// MessageReceived
bool
ScheduleItemManipulator::MessageReceived(BMessage* message, Command** _command)
{
	if (message->what != MSG_SCHEDULE_ITEM_CHANGED)
		return false;

	const Observable* object;
	if (message->FindPointer("object", (void**)&object) != B_OK
		|| object != fItem)
		return false;

	_ObjectChanged();
	return true;
}

// UpdateCursor
bool
ScheduleItemManipulator::UpdateCursor()
{
	if (fPreparedToResize) {
		BCursor cursor(kUpDownCursor);
		fView->SetViewCursor(&cursor, true);
		return true;
	}
	return false;
}

// Bounds
BRect
ScheduleItemManipulator::Bounds()
{
	return fItemFrame;
}

// AttachedToView
void
ScheduleItemManipulator::AttachedToView(StateView* view)
{
	fView = dynamic_cast<ScheduleView*>(view);
	fItemFrame = _ComputeFrameFor(fItem);
}

// DetachedFromView
void
ScheduleItemManipulator::DetachedFromView(StateView* view)
{
	fView = NULL;
}

// RebuildCachedData
void
ScheduleItemManipulator::RebuildCachedData()
{
	BRect oldItemFrame = fItemFrame;

	fItemFrame = _ComputeFrameFor(fItem);
	fView->Invalidate(oldItemFrame | fItemFrame);
}

// SetHighlighted
void
ScheduleItemManipulator::SetHighlighted(bool highlighted, bool invalidate)
{
	if (fHighlighted == highlighted)
		return;

	fHighlighted = highlighted;

	if (invalidate)
		fView->Invalidate(fItemFrame);
}

// #pragma mark -

// _ObjectChanged
void
ScheduleItemManipulator::_ObjectChanged()
{
	fCachedStartFrame = fItem->StartFrame();
	fCachedDuration = fItem->Duration();
	fCachedSelected = fItem->IsSelected();

	fCachedFlexibleDuration = fItem->FlexibleDuration();

	fCachedName = fItem->Name();

	ScheduleItemManipulator::RebuildCachedData();
		// skip derived classes implementation
}

// _ComputeFrameFor
BRect
ScheduleItemManipulator::_ComputeFrameFor(ScheduleItem* item) const
{
	BRect frame;
		// an invalid frame

	if (!fView || !item)
		return frame;

	return fView->LayoutScheduleItem(item);
}


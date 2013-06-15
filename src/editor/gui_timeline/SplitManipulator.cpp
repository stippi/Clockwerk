/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SplitManipulator.h"

#include <new>
#include <stdio.h>

#include <Font.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <String.h>

#include "support.h"
#include "support_ui.h"

#include "ClipPlaylistItem.h"
#include "MainWindow.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "PropertyManipulator.h"
#include "TimelineMessages.h"
#include "TimelineView.h"

using std::nothrow;

// constructor
SplitManipulator::SplitManipulator(PlaylistItem* item)
	: PlaylistItemManipulator(item),
	  fPropertyManipulator(new (nothrow) PropertyManipulator(this,
			item->AlphaAnimator())),
	  fTrackingProperty(false),

	  fSnapFrames()
{
}

// destructor
SplitManipulator::~SplitManipulator()
{
	delete fPropertyManipulator;
}

// #pragma mark -

// Draw
void
SplitManipulator::Draw(BView* into, BRect updateRect)
{
	if (fItemFrame.IsValid()) {
		into->SetDrawingMode(B_OP_COPY);
		into->SetHighColor(0, 0, 0, 255);
		BRect r(fItemFrame);
		into->StrokeRect(r);
		if (r.IntegerWidth() < 3)
			return;

		into->SetDrawingMode(B_OP_ALPHA);
		into->SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);

		rgb_color base;
		if (ToolIsActive())
			base = (rgb_color){ 200, 200, 200, 220 };
		else {
			base = (rgb_color){ 230, 230, 230, 200 };
			if (fItem && fItem->IsSelected())
				base = tint_color(base, B_DARKEN_1_TINT);
		}

		into->SetHighColor(base);
		into->SetLowColor(base);

		rgb_color lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
		rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

		r = _UpperFrame();
		r.InsetBy(1, 1);

		into->BeginLineArray(3);
			into->AddLine(BPoint(r.left, r.bottom - 1),
						  BPoint(r.left, r.top), lighten2);
			into->AddLine(BPoint(r.left + 1, r.top),
						  BPoint(r.right - 1, r.top), lighten2);
			into->AddLine(BPoint(r.right, r.top),
						  BPoint(r.right, r.bottom - 1), darken2);
		into->EndLineArray();

		r.InsetBy(1, 1);
		into->FillRect(r);
		if (r.IntegerWidth() == 0)
			return;

		// clip name
		BString truncated(fItem->Name());
		BFont font;
		into->GetFont(&font);

		if (fItem->IsVideoMuted() || fItem->IsAudioMuted()) {
			// TODO: icons for displaying this info instead of font
			font.SetFace(B_ITALIC_FACE);
			into->SetFont(&font, B_FONT_FACE);
		}

		font.TruncateString(&truncated, B_TRUNCATE_END, r.Width() - 10.0);
		font_height fh;
		font.GetHeight(&fh);
		BPoint textPos;
		textPos.x = r.left + 5.0;
		textPos.y = (r.top + r.bottom + fh.ascent) / 2.0 - 1.0;

		into->SetHighColor(0, 0, 0, 200);
		into->DrawString(truncated.String(), textPos);

		// let the tool manipulator draw
		ToolDraw(into, r);

		// fill lower half for property display
		r.left = fItemFrame.left + 1;
		r.right = fItemFrame.right - 1;
		r.top = r.bottom + 1;
		r.bottom = fItemFrame.bottom - 1;

		if (ToolIsActive())
			base = (rgb_color){ 205, 205, 205, 220 };
		else {
			base = (rgb_color){ 235, 235, 235, 220 };
			if (fItem && fItem->IsSelected())
				base = tint_color(base, B_DARKEN_1_TINT);
		}

		into->SetHighColor(base);

		lighten2 = tint_color(base, B_LIGHTEN_2_TINT);
		rgb_color darken1 = tint_color(base,
									   (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
		darken2 = tint_color(base, B_DARKEN_1_TINT);

		into->BeginLineArray(4);
			into->AddLine(BPoint(r.left, r.bottom),
						  BPoint(r.left, r.top + 1), lighten2);
			into->AddLine(BPoint(r.left, r.top),
						  BPoint(r.right - 1, r.top), darken1);
			into->AddLine(BPoint(r.right, r.top),
						  BPoint(r.right, r.bottom), darken2);
			into->AddLine(BPoint(r.right - 1, r.bottom),
						  BPoint(r.left + 1, r.bottom), darken2);
		into->EndLineArray();

		r.InsetBy(1, 1);
		into->FillRect(r);

		// let the property manipulator draw
		if (fPropertyManipulator)
			fPropertyManipulator->Draw(into, updateRect);
	}
}

// MouseDown
bool
SplitManipulator::MouseDown(BPoint where)
{
	if (fItem && fItemFrame.Contains(where)) {
		// Clicking in the upper half of the item
		// is handled by the tool, then, if clicking
		// in the lower half is not handled by the
		// PropertyManipulator, it can still be
		// handled by the tool.
		fTrackingProperty = false;

		if (fPropertyManipulator->MouseDown(where)) {
			fTrackingProperty = true;
			return true;
		}
		fView->Select(fItem);

		return ToolMouseDown(where);
	}
	return false;
}

// MouseMoved
void
SplitManipulator::MouseMoved(BPoint where)
{
	if (!fItem)
		return;

	if (fTrackingProperty)
		fPropertyManipulator->MouseMoved(where);
	else
		ToolMouseMoved(where);
}

// MouseUp
Command*
SplitManipulator::MouseUp()
{
	if (fTrackingProperty)
		return fPropertyManipulator->MouseUp();

	Command* command = ToolMouseUp();
	fView->Invalidate(fItemFrame);
	return command;
}

// MouseOver
bool
SplitManipulator::MouseOver(BPoint where)
{
	if (fPropertyManipulator->MouseOver(where))
		return true;

	return ToolMouseOver(where);
}

// DisplayPopupMenu
bool
SplitManipulator::DisplayPopupMenu(BPoint where)
{
	BPopUpMenu* menu = new BPopUpMenu("item popup", false, false);

	BMessage* message;
	BMenuItem* item;

	bool separator = false;

	if (fItem->HasVideo()) {
		message = new BMessage(MSG_SET_VIDEO_MUTED);
		message->AddPointer("item", fItem);
		item = new BMenuItem("Enabled Video", message);
		item->SetMarked(!fItem->IsVideoMuted());
		menu->AddItem(item);
		separator = true;
	}

	if (fItem->HasAudio()) {
		message = new BMessage(MSG_SET_AUDIO_MUTED);
		message->AddPointer("item", fItem);
		item = new BMenuItem("Enabled Audio", message);
		item->SetMarked(!fItem->IsAudioMuted());
		menu->AddItem(item);
		separator = true;
	}

	if (separator)
		menu->AddSeparatorItem();

	BMenuItem* selectClipItem = NULL;
	if (ClipPlaylistItem* clipItem = dynamic_cast<ClipPlaylistItem*>(fItem)) {
		message = new BMessage(MSG_SELECT_AND_SHOW_CLIP);
		message->AddPointer("clip", clipItem->Clip());
		selectClipItem = new BMenuItem("Select Clip", message);
		menu->AddItem(selectClipItem);

		if (clipItem->NavigationInfo()) {
			message = new BMessage(MSG_EDIT_NAVIGATOR_INFO);
			message->AddPointer("item", fItem);
			item = new BMenuItem("Edit Navigation Info" B_UTF8_ELLIPSIS,
				message);
			menu->AddItem(item);

			message = new BMessage(MSG_REMOVE_NAVIGATOR_INFO);
			message->AddPointer("item", fItem);
			item = new BMenuItem("Remove Navigation Info", message);
			menu->AddItem(item);
		} else {
			message = new BMessage(MSG_ADD_NAVIGATOR_INFO);
			message->AddPointer("item", fItem);
			item = new BMenuItem("Add Navigation Info" B_UTF8_ELLIPSIS,
				message);
			menu->AddItem(item);
		}
	} else
		separator = false;

	if (separator)
		menu->AddSeparatorItem();

	message = new BMessage(MSG_REMOVE_ITEM);
	message->AddPointer("item", fItem);
	item = new BMenuItem("Remove Clip", message);
	menu->AddItem(item);

	menu->SetTargetForItems(fView);
	if (selectClipItem)
		selectClipItem->SetTarget(fView->Window());

	show_popup_menu(menu, where, fView, false);

	return true;
}

// RebuildCachedData
void
SplitManipulator::RebuildCachedData()
{
	PlaylistItemManipulator::RebuildCachedData();

	// pass on the event to the child manipulator
	fPropertyManipulator->RebuildCachedData();
}

// #pragma mark -

// ToolDraw
void
SplitManipulator::ToolDraw(BView* into, BRect itemFrame)
{
	// default implementation is empty
}

// #pragma mark -

// _UpperFrame
BRect
SplitManipulator::_UpperFrame() const
{
	BRect r(fItemFrame);
	r.bottom = roundf((r.top + r.bottom) / 2.0);
	return r;
}


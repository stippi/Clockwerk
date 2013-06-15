/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TrackView.h"

#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Window.h>

#include "CommandStack.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "TextViewPopup.h"
#include "TimelineView.h"
#include "TrackProperties.h"

#include "CompoundCommand.h"
#include "DeleteCommand.h"
#include "InsertOrRemoveTrackCommand.h"
#include "MoveTrackCommand.h"
#include "SetSoloTrackCommand.h"
#include "SetTrackPropertiesCommand.h"

enum {
	MSG_EDIT_TRACK_NAME	= 'etrn',
	MSG_INSERT_TRACK	= 'intr',
	MSG_REMOVE_TRACK	= 'rmtr',
};

// constructor
TrackView::TrackView(BRect frame)
	: BView(frame, "track view", B_FOLLOW_ALL, B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
	, fPlaylist(NULL)
	, fCommandStack(NULL)
	, fEditedTrackName(-1)
	, fDraggedTrack(-1)
{
	SetViewColor(B_TRANSPARENT_COLOR);

	GetFont(&fFont);

	fButtonFont = fFont;
	fButtonFont.SetSize(max_c(9.0, fButtonFont.Size() * 0.8));

	font_height fh;
	fFont.GetHeight(&fh);
	fAscent = fh.ascent;

	fMuteWidth = fButtonFont.StringWidth("Mute");
	fSoloWidth = fButtonFont.StringWidth("Solo");
	fButtonWidth = max_c(fMuteWidth, fSoloWidth) + 16;

	fButtonFont.GetHeight(&fh);
	fSmallAscent = fh.ascent;
}

// destructor
TrackView::~TrackView()
{
	SetPlaylist(NULL);
}

// Draw
void
TrackView::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color darken1 = tint_color(base, B_DARKEN_1_TINT);
	rgb_color darken2 = tint_color(darken1, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);
	rgb_color darken3 = tint_color(base, B_DARKEN_3_TINT);
	rgb_color darkenMax = tint_color(darken1, B_DARKEN_4_TINT);
	rgb_color baseSoft = tint_color(base, (B_NO_TINT + B_DARKEN_1_TINT) / 2.0);

	SetLowColor(darken1);
	FillRect(updateRect, B_SOLID_LOW);

	if (!fPlaylist)
		return;

	BRect r(Bounds());

	int32 trackHeight = fTimelineView->TrackHeight();
	int32 firstTrack = (int32)r.top / trackHeight;
	int32 lastTrack = (int32)r.bottom / trackHeight + 1;

	for (int32 i = firstTrack; i <= lastTrack; i++) {
		BRect tr(r.left, i * trackHeight, r.right, (i + 1) * trackHeight - 1);
		if (!tr.Intersects(updateRect))
			continue;

		BeginLineArray(4);
			AddLine(BPoint(tr.left, tr.top),
					BPoint(tr.right, tr.top), darken3);
			AddLine(BPoint(tr.left, tr.top + 1),
					BPoint(tr.right, tr.top + 1), base);
			AddLine(BPoint(tr.left, tr.bottom),
					BPoint(tr.right, tr.bottom), darken2);
			AddLine(BPoint(tr.left, tr.bottom + 1),
					BPoint(tr.right, tr.bottom + 1), darken3);
		EndLineArray();

		// fetch track settings
		BString trackName;
		TrackProperties* properties = fPlaylist->PropertiesForTrack(i);
		bool muted = false;
		bool solo = fPlaylist->SoloTrack() == i;
		if (properties) {
			trackName = properties->Name();
			muted = !properties->IsEnabled();
		}
		if (trackName.Length() == 0)
			_GenerateTrackName(trackName, i);

		if (i != fEditedTrackName) {
			// draw name
			SetFont(&fFont);
			SetHighColor(darkenMax);
			SetLowColor(darken1);
			BPoint pos(tr.left + 5, tr.top + (tr.Height() + fAscent) / 2.0);
			BString truncatedName(trackName);
			fFont.TruncateString(&truncatedName, B_TRUNCATE_MIDDLE,
								 tr.Width() - fButtonWidth - 10);
			DrawString(truncatedName.String(), pos);
		}

		// draw mute and solo buttons
		SetFont(&fButtonFont);
		tr.InsetBy(4, 4);

		// draw solo button
		BRect buttonRect = tr;
		buttonRect.bottom = ceilf((tr.top + tr.bottom) / 2);
		buttonRect.left = tr.right - fButtonWidth;
		rgb_color fill = solo ? (rgb_color){ 255, 236, 169, 255 } : baseSoft;

		_DrawButton(buttonRect, "Solo", fSmallAscent, fSoloWidth, base, fill, true);

		// draw mute button
		buttonRect.top = buttonRect.bottom + 1;
		buttonRect.bottom = tr.bottom;
		if (muted && !solo)
			fill = (rgb_color){ 236, 179, 124, 255 };
		else if (muted && solo)
			fill = base;
		else
			fill = baseSoft;

		_DrawButton(buttonRect, "Mute", fSmallAscent, fMuteWidth, base, fill, false);
	}
}

// MessageReceived
void
TrackView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_EDIT_TRACK_NAME: {
			int32 track;
			const char* text;
			if (fPlaylist && fCommandStack
				&& message->FindInt32("track", &track) == B_OK
				&& message->FindString("text", &text) == B_OK) {
				// apply new track name
				TrackProperties oldProperties(track);
				const TrackProperties* original
					= fPlaylist->PropertiesForTrack(track);
				if (original)
					oldProperties = *original;

				TrackProperties newProperties(oldProperties);
				newProperties.SetName(BString(text));
				fCommandStack->Perform(new SetTrackPropertiesCommand(fPlaylist,
					oldProperties, newProperties));

				// in case user used TAB navigation, edit the next track name
				int32 next;
				if (message->FindInt32("next", &next) == B_OK
					&& (track + next >= 0)) {
					_EditTrackName(track + next);
				} else {
					// make sure we draw the track name again
					fEditedTrackName = -1;
				}
			} else {
				// editing canceled or some other error,
				// make sure we redraw the track name
				Invalidate(_TrackRect(fEditedTrackName));
				fEditedTrackName = -1;
			}
			break;
		}
		case MSG_INSERT_TRACK: {
			int32 track;
			if (fPlaylist && fCommandStack
				&& message->FindInt32("track", &track) == B_OK
				&& track >= 0) {
				fCommandStack->Perform(
					new InsertOrRemoveTrackCommand(fPlaylist, track, true));
			}
			break;
		}
		case MSG_REMOVE_TRACK: {
			uint32 track;
			if (fPlaylist && fCommandStack
				&& message->FindInt32("track", (int32*)&track) == B_OK
				&& track >= 0) {

				// collect playlist items
				BList itemsOnTrack;
				int32 count = fPlaylist->CountItems();
				bool error = false;
				for (int32 i = 0; i < count; i++) {
					PlaylistItem* item = fPlaylist->ItemAtFast(i);
					if (item->Track() == track && !itemsOnTrack.AddItem(item)) {
						error = true;
						break;
					}
				}
				if (error)
					break;

				count = itemsOnTrack.CountItems();
				if (count > 0) {
					// form compound delete+remove command
					PlaylistItem** items
						= new (std::nothrow) PlaylistItem*[count];
					Command** commands = new (std::nothrow) Command*[2];
					if (!items || !commands) {
						delete[] items;
						delete[] commands;
						break;
					}
					memcpy(items, itemsOnTrack.Items(),
						   count * sizeof(PlaylistItem*));
					commands[0]
						= new DeleteCommand(fPlaylist, items, count, NULL);
					commands[1]
						= new InsertOrRemoveTrackCommand(fPlaylist, track,
							false);
					BString commandName;
					commands[1]->GetName(commandName);
					fCommandStack->Perform(
						new CompoundCommand(commands, 2, commandName.String(), 0));
				} else {
					fCommandStack->Perform(
						new InsertOrRemoveTrackCommand(fPlaylist, track,
							false));
				}
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}

// MouseDown
void
TrackView::MouseDown(BPoint where)
{
	if (!fPlaylist || !fTimelineView || !fCommandStack)
		return;

	int32 trackHeight = fTimelineView->TrackHeight();
	int32 track = (int32)where.y / trackHeight;

	uint32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons) < B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;
	if (buttons & B_SECONDARY_MOUSE_BUTTON) {
		// display context menu
		BPopUpMenu* menu = new BPopUpMenu("track popup", false, false);

		BMessage* message = new BMessage(MSG_INSERT_TRACK);
		message->AddInt32("track", track);
		BMenuItem* item = new BMenuItem("Insert New Track Here", message);
		menu->AddItem(item);

		message = new BMessage(MSG_REMOVE_TRACK);
		message->AddInt32("track", track);
		item = new BMenuItem("Remove This Track", message);
		menu->AddItem(item);

		menu->SetTargetForItems(this);
		menu->SetAsyncAutoDestruct(true);
		menu->SetFont(be_plain_font);

		where = ConvertToScreen(where);
		BRect mouseRect(where, where);
		mouseRect.InsetBy(-10.0, -10.0);
		where += BPoint(5.0, 5.0);
		menu->Go(where, true, false, mouseRect, true);

		return;
	}

	BRect trackRect = _TrackRect(track);
	if (_SoloRect(trackRect).Contains(where)) {
		if (fPlaylist->SoloTrack() != track)
			fCommandStack->Perform(new SetSoloTrackCommand(fPlaylist, track));
		else
			fCommandStack->Perform(new SetSoloTrackCommand(fPlaylist, -1));
	} else if (_MuteRect(trackRect).Contains(where)) {
		TrackProperties oldProperties(track);
		const TrackProperties* original = fPlaylist->PropertiesForTrack(track);
		if (original)
			oldProperties = *original;

		TrackProperties newProperties(oldProperties);
	
		newProperties.SetEnabled(!newProperties.IsEnabled());
		fCommandStack->Perform(new SetTrackPropertiesCommand(fPlaylist,
			oldProperties, newProperties));
	} else {
		// test if click on track name -> edit it
		trackRect.top = (trackRect.top + trackRect.bottom - fAscent) / 2.0 - 2;
		trackRect.bottom = trackRect.top + fAscent + 6;
		BString name = _GetTrackName(track);
		trackRect.right = trackRect.left + 8 + StringWidth(name.String());
		if (trackRect.Contains(where)) {
			// TODO: delay popup, react on mouse being dragged outside text
			// after button is released...
			_EditTrackName(track);
		} else {
			// begin dragging this track
			fDraggedTrack = track;
		}
	}
}

// MouseUp
void
TrackView::MouseUp(BPoint where)
{
	fDraggedTrack = -1;
}

// MouseMoved
void
TrackView::MouseMoved(BPoint where, uint32 transit,
					  const BMessage* dragMessage)
{
	if (fDraggedTrack < 0 || !fCommandStack || !fPlaylist || !fTimelineView)
		return;

	int32 trackHeight = fTimelineView->TrackHeight();
	int32 track = (int32)where.y / trackHeight;
	if (track < 0 || fDraggedTrack == track)
		return;

	fCommandStack->Perform(new MoveTrackCommand(fPlaylist, fDraggedTrack, track));
	fDraggedTrack = track;
}

// #pragma mark -

// TrackPropertiesChanged
void
TrackView::TrackPropertiesChanged(Playlist* playlist, uint32 track)
{
	if (!LockLooper())
		return;

	Invalidate(_TrackRect(track));

	UnlockLooper();
}

// TrackMoved
void
TrackView::TrackMoved(Playlist* playlist, uint32 oldIndex, uint32 newIndex)
{
	if (!LockLooper())
		return;

	Invalidate(_TrackRect(oldIndex) | _TrackRect(newIndex));

	UnlockLooper();
}

// TrackInserted
void
TrackView::TrackInserted(Playlist* playlist, uint32 track)
{
	if (!LockLooper())
		return;

	BRect dirty = _TrackRect(track);
	dirty.bottom = Bounds().bottom;
	Invalidate(dirty);

	UnlockLooper();
}

// TrackRemoved
void
TrackView::TrackRemoved(Playlist* playlist, uint32 track)
{
	if (!LockLooper())
		return;

	BRect dirty = _TrackRect(track);
	dirty.bottom = Bounds().bottom;
	Invalidate(dirty);

	UnlockLooper();
}

// #pragma mark -

// SetPlaylist
void
TrackView::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	if (fPlaylist)
		fPlaylist->RemoveListObserver(this);

	fPlaylist = playlist;

	if (fPlaylist)
		fPlaylist->AddListObserver(this);

	fEditedTrackName = -1;
	fDraggedTrack = -1;

	Invalidate();
}

// SetCommandStack
void
TrackView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetTimelineView
void
TrackView::SetTimelineView(TimelineView* view)
{
	fTimelineView = view;
}

// #pragma mark -

// _TrackRect
BRect
TrackView::_TrackRect(uint32 track) const
{
	BRect rect;
	if (fTimelineView) {
		rect = Bounds();
		int32 trackHeight = fTimelineView->TrackHeight();
		rect.top = track * trackHeight;
		rect.bottom = (track + 1) * trackHeight - 1;
	}
	return rect;
}

// _SoloRect
BRect
TrackView::_SoloRect(BRect trackRect) const
{
	trackRect.InsetBy(4, 4);
	trackRect.bottom = ceilf((trackRect.top + trackRect.bottom) / 2);
	trackRect.left = trackRect.right - fButtonWidth;
	return trackRect;
}

// _MuteRect
BRect
TrackView::_MuteRect(BRect trackRect) const
{
	trackRect.InsetBy(4, 4);
	trackRect.top = ceilf((trackRect.top + trackRect.bottom) / 2) + 1;
	trackRect.left = trackRect.right - fButtonWidth;
	return trackRect;
}

// _DrawButton
void
TrackView::_DrawButton(BRect r, const char* label, float ascent, float width,
					   rgb_color base, rgb_color fill, bool top)
{
	rgb_color baseDarken1 = tint_color(base, (B_DARKEN_1_TINT + B_DARKEN_2_TINT) / 2);
	rgb_color baseDarken = tint_color(base, B_DARKEN_4_TINT);
	rgb_color baseLighten = tint_color(base, (B_NO_TINT + B_DARKEN_1_TINT) / 2);
	rgb_color fillDarken = tint_color(fill, B_DARKEN_1_TINT);
	rgb_color fillLighten = tint_color(fill, B_LIGHTEN_2_TINT);
	rgb_color fillText = tint_color(fill, (B_DARKEN_4_TINT + B_DARKEN_MAX_TINT) / 2);

	BeginLineArray(top ? 11 : 10);
		// outer bevel edge
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), baseDarken1);
		if (top) {
			AddLine(BPoint(r.left + 1, r.top),
					BPoint(r.right, r.top), baseDarken1);
			AddLine(BPoint(r.right, r.top + 1),
					BPoint(r.right, r.bottom), baseLighten);
			r.InsetBy(1, 1);
			r.bottom++;
		} else {
			AddLine(BPoint(r.right, r.top),
					BPoint(r.right, r.bottom), baseLighten);
			AddLine(BPoint(r.right - 1, r.bottom),
					BPoint(r.left + 1, r.bottom), baseLighten);
			r.InsetBy(1, 1);
			r.top--;
		}

		// dark edge
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), baseDarken);
		if (top) {
			AddLine(BPoint(r.left + 1, r.top),
					BPoint(r.right, r.top), baseDarken);
			AddLine(BPoint(r.right, r.top + 1),
					BPoint(r.right, r.bottom), baseDarken);
			AddLine(BPoint(r.right - 1, r.bottom),
					BPoint(r.left + 1, r.bottom), baseDarken);
			r.InsetBy(1, 1);
		} else {
			AddLine(BPoint(r.right, r.top),
					BPoint(r.right, r.bottom), baseDarken);
			AddLine(BPoint(r.right - 1, r.bottom),
					BPoint(r.left + 1, r.bottom), baseDarken);
			r.InsetBy(1, 1);
			r.top--;
		}

		// fill edge
		AddLine(BPoint(r.left, r.bottom - 1),
				BPoint(r.left, r.top), fillLighten);
		AddLine(BPoint(r.left + 1, r.top),
				BPoint(r.right, r.top), fillLighten);
		AddLine(BPoint(r.right, r.top + 1),
				BPoint(r.right, r.bottom), fillDarken);
		AddLine(BPoint(r.right - 1, r.bottom),
				BPoint(r.left, r.bottom), fillDarken);
		r.InsetBy(1, 1);
	EndLineArray();

	SetLowColor(fill);
	SetHighColor(fillText);
	FillRect(r, B_SOLID_LOW);
	DrawString(label, BPoint((r.left + r.right - width) / 2,
							 floorf((r.top + r.bottom + ascent) / 2.0)));
}

// _GetTrackName
BString
TrackView::_GetTrackName(uint32 track, bool generate) const
{
	BString name;
	if (TrackProperties* properties = fPlaylist->PropertiesForTrack(track))
		name = properties->Name();
	if (generate && name.Length() == 0)
		_GenerateTrackName(name, track);
	return name;
}

// _GenerateTrackName
void
TrackView::_GenerateTrackName(BString& name, uint32 track) const
{
	name << "<track " << track + 1 << ">";
}

// _EditTrackName
void
TrackView::_EditTrackName(uint32 track)
{
	BRect frame = _TrackRect(track);

	frame.left += 2;
	frame.top = floorf((frame.top + frame.bottom - fAscent) / 2.0) - 1;
	frame.bottom = ceilf(frame.top + fAscent) + 4;
	frame.right = frame.right - fButtonWidth - 5;

	fEditedTrackName = track;
	Invalidate(frame);

	ConvertToScreen(&frame);

	BMessage* message = new BMessage(MSG_EDIT_TRACK_NAME);
	message->AddInt32("track", track);

	BString name(_GetTrackName(track, false));
	new TextViewPopup(frame, name, message, this);
}



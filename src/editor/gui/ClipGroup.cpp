/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipGroup.h"

#include <stdio.h>

#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <TextControl.h>
#include <Window.h>

#include "common_constants.h"
#include "ui_defines.h"

#include "BitmapClip.h"
#include "ClockClip.h"
#include "ColorClip.h"
#include "CommonPropertyIDs.h"
#include "MediaClip.h"
#include "Playlist.h"
#include "ScrollingTextClip.h"
#include "TableClip.h"
#include "TextClip.h"
#include "TimerClip.h"
#include "WeatherClip.h"

enum {
	MSG_SET_CLIP_TYPE			= 'stcp',
	MSG_TOGGLE_PLAYLIST_CLIPS	= 'tgpc',
	MSG_TOGGLE_NAME_FILTER		= 'tgnf',
	MSG_SET_NAME_FILTER			= 'stnf',
};

class FocusChangeReportingTextControl : public BTextControl {
public:
			FocusChangeReportingTextControl(
					const char* name, const char* label,
					const char* initialText, BMessage* message,
					uint32 flags = B_WILL_DRAW | B_NAVIGABLE)
				:
				BTextControl(name, label, initialText, message, flags),
				fWasFocus(false)
			{
			}

	virtual	void Draw(BRect updateRect)
			{
				// NOTE: there seems to be no other way of catching the
				// focus change of the child BTextView...
				if (fWasFocus != TextView()->IsFocus()) {
					fWasFocus = TextView()->IsFocus();
					if (Window()) {
						BMessage focusMessage(MSG_FOCUS_CHANGED);
						focusMessage.AddPointer("source", TextView());
						focusMessage.AddBool("focus", fWasFocus);
						Window()->PostMessage(&focusMessage, Window());
					}
				}

				BTextControl::Draw(updateRect);
			}

	bool	fWasFocus;
};


class ClipListScrollView : public BScrollView {
public:
	ClipListScrollView(BView* target)
		:
		BScrollView("list scroll view",
			target, 0, false, true, B_NO_BORDER)
	{
	}

protected:
	virtual void DoLayout()
	{
		BScrollView::DoLayout();
		if (BScrollBar* scrollBar = ScrollBar(B_VERTICAL)) {
			scrollBar->MoveBy(0, -1);
			scrollBar->ResizeBy(0, 2);
		}
	}
};


// constructor
ClipGroup::ClipGroup(ClipListView* listView)
	:
	BView("clip group", B_FRAME_EVENTS),
	fClipListView(listView),
	fPreviousBounds(Bounds())
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	fClipTypeM = _CreateTypeMenu();

	// type menu field
	fClipTypeMF = new BMenuField("show type", "Show Type", fClipTypeM, NULL);

	// playlist clips only checkbox
	fPlaylistClipsOnlyCB = new BCheckBox("show pl clips",
		"Only clips used in current playlist",
		new BMessage(MSG_TOGGLE_PLAYLIST_CLIPS));

	// filter by "name contains" checkbox
	fFilterByNameCB = new BCheckBox("filter by name",
		"Filter by Name Contains", new BMessage(MSG_TOGGLE_NAME_FILTER));
	fFilterByNameCB->SetValue(false);

	// "name contains" text control
	fNameContainsTC = new FocusChangeReportingTextControl(
		"name contains", NULL, "", new BMessage(MSG_SET_NAME_FILTER));
	fNameContainsTC->SetEnabled(false);

	// clip list view
	BScrollView* scrollView = new ClipListScrollView(fClipListView);

	const float spacing = 5.0f;
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL, 0.0f);
	SetLayout(layout);
	BGroupLayoutBuilder(layout)
		.AddGroup(B_VERTICAL, 2.0f)
			.SetInsets(spacing, spacing, 0, spacing)
			.Add(fClipTypeMF)
			.Add(fPlaylistClipsOnlyCB)
			.AddGroup(B_HORIZONTAL, spacing)
				.Add(fFilterByNameCB)
				.Add(fNameContainsTC)
			.End()
		.End()
		.AddGroup(B_HORIZONTAL, 0.0f)
			.AddGroup(B_VERTICAL, 0.0f)
				.Add(new BSeparatorView("", B_HORIZONTAL, B_PLAIN_BORDER))
				.Add(scrollView)
			.End()
			.AddStrut(0.0f)
		.End()
	;
}

// destructor
ClipGroup::~ClipGroup()
{
}

// AttachedToWindow
void
ClipGroup::AttachedToWindow()
{
	fClipTypeM->SetTargetForItems(this);
	fPlaylistClipsOnlyCB->SetTarget(this);
	fFilterByNameCB->SetTarget(this);
	fNameContainsTC->SetTarget(this);
}

// FrameResized
void
ClipGroup::FrameResized(float width, float height)
{
	BRect dirty = fPreviousBounds;
	fPreviousBounds = Bounds();
	dirty = dirty & fPreviousBounds;
	dirty.left = dirty.right - 20;
}

// MessageReceived
void
ClipGroup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_CLIP_TYPE:
		{
			int32 type;
			if (message->FindInt32("type", &type) == B_OK)
				fClipListView->SetItemSorter(_SorterForType(type));
			break;
		}

		case MSG_TOGGLE_NAME_FILTER:
			_SetFilterByNameEnabled(fFilterByNameCB->Value() == B_CONTROL_ON);
			// fall through
		case MSG_SET_NAME_FILTER:
			if (fClipListView->NameContainsFilterString()
					!= _NameContainsFilterString()) {
				fClipListView->SetItemSorter(_SorterForType(ClipType()));
			}
			break;
		case MSG_TOGGLE_PLAYLIST_CLIPS:
			fClipListView->SetItemSorter(_SorterForType(ClipType()));
			break;

		default:
			BView::MessageReceived(message);
	}
}

// #pragma mark -

// SetClipType
void
ClipGroup::SetClipType(int32 type)
{
	if (type == ClipType())
		return;

	_SelectClipTypeItem(type);
	fClipListView->SetItemSorter(_SorterForType(type));
}

// ClipType
int32
ClipGroup::ClipType() const
{
	BMenuItem* item = fClipTypeM->FindMarked();
	if (!item)
		return CLIP_TYPE_ALL;

	int32 type;
	if (item->Message()->FindInt32("type", &type) < B_OK)
		return CLIP_TYPE_ALL;

	return type;
}

// SetPlaylistClipsOnly
void
ClipGroup::SetPlaylistClipsOnly(bool playlistOnly)
{
	if (playlistOnly == PlaylistClipsOnly())
		return;

	fPlaylistClipsOnlyCB->SetValue(playlistOnly ? B_CONTROL_ON : B_CONTROL_OFF);
	fClipListView->SetItemSorter(_SorterForType(ClipType()));
}

// PlaylistClipsOnly
bool
ClipGroup::PlaylistClipsOnly() const
{
	return fPlaylistClipsOnlyCB->Value() == B_CONTROL_ON;
}

// SetNameContainsOnly
void
ClipGroup::SetNameContainsOnly(bool nameContainsOnly)
{
	if (nameContainsOnly == NameContainsOnly())
		return;

	_SetFilterByNameEnabled(nameContainsOnly);
	fClipListView->SetItemSorter(_SorterForType(ClipType()));
}

// NameContainsOnly
bool
ClipGroup::NameContainsOnly() const
{
	return fFilterByNameCB->Value() == B_CONTROL_ON;
}

// SetNameContainsString
void
ClipGroup::SetNameContainsString(const char* string)
{
	fNameContainsTC->SetText(string);
	fClipListView->SetItemSorter(_SorterForType(ClipType()));
}

// NameContainsString
const char*
ClipGroup::NameContainsString() const
{
	return fNameContainsTC->Text();
}

// #pragma mark -

// MakeSureClipShows
void
ClipGroup::MakeSureClipShows(Clip* clip)
{
	if (!clip)
		return;

	bool switchSorter = false;

	if (NameContainsOnly()
		&& clip->Name().FindFirst(fNameContainsTC->Text()) < 0) {
		_SetFilterByNameEnabled(false);
//printf("disabling filter\n");
		switchSorter = true;
	}

	uint32 type = _TypeForClip(clip);
	// do not make type more specific than necessary
	// only change it if the clip would not show
	uint32 currentType = ClipType();
	if (currentType == CLIP_TYPE_ALL) {
		type = currentType;
	} else if (currentType == CLIP_TYPE_ALL_FOR_UPLOAD
		&& (clip->Status() == SYNC_STATUS_LOCAL
			|| clip->Status() == SYNC_STATUS_MODIFIED)) {
		type = currentType;
	} else if (currentType == CLIP_TYPE_ALL_NEW
		&& clip->Status() == SYNC_STATUS_LOCAL) {
		type = currentType;
	}

	if (currentType != type) {
		_SelectClipTypeItem(type);
//printf("switching type\n");
		switchSorter = true;
	}

	if (switchSorter)
		fClipListView->SetItemSorter(_SorterForType(type));

	int32 index = fClipListView->IndexForClip(clip);
	if (index < 0) {
		if (fPlaylistClipsOnlyCB->Value() != B_CONTROL_OFF) {
//printf("disabling playlist only\n");
			fPlaylistClipsOnlyCB->SetValue(B_CONTROL_OFF);
			fClipListView->SetItemSorter(_SorterForType(type));
			index = fClipListView->IndexForClip(clip);
		}
	}

	if (index >= 0)
		fClipListView->ScrollTo(index);
}

// #pragma mark -

BMenu*
ClipGroup::_CreateTypeMenu() const
{
	BPopUpMenu* menu = new BPopUpMenu("types");
	// items
	BMenuItem* item;
	BMessage* message;

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_ALL);
	item = new BMenuItem("All", message);
	menu->AddItem(item);
	item->SetMarked(true);

	menu->AddSeparatorItem();

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_BITMAP);
	item = new BMenuItem("Bitmaps", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_CLOCK);
	item = new BMenuItem("Clocks", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_COLOR);
	item = new BMenuItem("Colors", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_MEDIA);
	item = new BMenuItem("Media Clips", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_PLAYLIST);
	item = new BMenuItem("Playlists", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_TABLE);
	item = new BMenuItem("Tables", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_TICKER);
	item = new BMenuItem("Tickers", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_TIMER);
	item = new BMenuItem("Timers", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_TEXT);
	item = new BMenuItem("Texts", message);
	menu->AddItem(item);

	message = new BMessage(MSG_SET_CLIP_TYPE);
	message->AddInt32("type", CLIP_TYPE_WEATHER);
	item = new BMenuItem("Weathers", message);
	menu->AddItem(item);

	return menu;
}

// _SelectClipTypeItem
void
ClipGroup::_SelectClipTypeItem(int32 type)
{
	for (int32 i = 0; BMenuItem* item = fClipTypeM->ItemAt(i); i++) {
		int32 itemType;
		if (item->Message()
			&& item->Message()->FindInt32("type", &itemType) == B_OK
			&& itemType == type) {
			item->SetMarked(true);
			break;
		}
	}
}

// _TypeForTypeString
uint32
ClipGroup::_TypeForTypeString(const BString& type) const
{
	if (type.FindFirst("Playlist") >= 0) {
		return CLIP_TYPE_PLAYLIST;
	} else if (type == "BitmapClip") {
		return CLIP_TYPE_BITMAP;
	} else if (type == "ClockClip") {
		return CLIP_TYPE_CLOCK;
	} else if (type == "ColorClip") {
		return CLIP_TYPE_COLOR;
	} else if (type == "MediaClip") {
		return CLIP_TYPE_MEDIA;
	} else if (type == "ScrollingTextClip") {
		return CLIP_TYPE_TICKER;
	} else if (type == "TextClip") {
		return CLIP_TYPE_TEXT;
	} else if (type == "TableClip") {
		return CLIP_TYPE_TABLE;
	} else if (type == "TimerClip") {
		return CLIP_TYPE_TIMER;
	} else if (type == "WeatherClip") {
		return CLIP_TYPE_WEATHER;
	}

	printf("ClipGroup::_TypeForTypeString(%s) - "
		"forgot to add cliptype!\n", type.String());

	return CLIP_TYPE_ALL;
}

// _TypeForClip
uint32
ClipGroup::_TypeForClip(const Clip* clip) const
{
	uint32 type;
	if (dynamic_cast<const BitmapClip*>(clip))
		type = _TypeForTypeString("BitmapClip");
	else if (dynamic_cast<const MediaClip*>(clip))
		type = _TypeForTypeString("MediaClip");
	else
		type = _TypeForTypeString(clip->Type());
	return type;
}

// _SetFilterByNameEnabled
void
ClipGroup::_SetFilterByNameEnabled(bool enabled)
{
	fFilterByNameCB->SetValue(enabled);
	fNameContainsTC->SetEnabled(enabled);
	fNameContainsTC->MakeFocus(enabled);
}

// _NameContainsFilterString
const char*
ClipGroup::_NameContainsFilterString() const
{
	if (fFilterByNameCB->Value() == B_CONTROL_ON)
		return fNameContainsTC->Text();
	return "";
}

class AllNonPublishedSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (clip->Status() != SYNC_STATUS_PUBLISHED)
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class AllNewSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (clip->Status() == SYNC_STATUS_LOCAL)
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class BitmapSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<BitmapClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class ClockSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<ClockClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class ColorSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<ColorClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class TimerSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<TimerClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class MediaSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<MediaClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class PlaylistSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<Playlist*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class TableSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<TableClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class TickerSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<ScrollingTextClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class TextSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<TextClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

class WeatherSorter : public ClipListView::ItemSorter {
	virtual	int32 IndexForClip(ClipListView* listView, Clip* clip) const
	{
		if (dynamic_cast<WeatherClip*>(clip))
			return ItemSorter::IndexForClip(listView, clip);
		return -1;
	}
};

// _SorterForType
ClipListView::ItemSorter*
ClipGroup::_SorterForType(int32 type)
{
	ClipListView::ItemSorter* sorter = NULL;
	switch (type) {
		case CLIP_TYPE_ALL:
			sorter = new ClipListView::ItemSorter;
			break;
		case CLIP_TYPE_ALL_FOR_UPLOAD:
			sorter = new AllNonPublishedSorter;
			break;
		case CLIP_TYPE_ALL_NEW:
			sorter = new AllNewSorter;
			break;

		case CLIP_TYPE_BITMAP:
			sorter = new BitmapSorter;
			break;
		case CLIP_TYPE_CLOCK:
			sorter = new ClockSorter;
			break;
		case CLIP_TYPE_COLOR:
			sorter = new ColorSorter;
			break;
		case CLIP_TYPE_MEDIA:
			sorter = new MediaSorter;
			break;
		case CLIP_TYPE_PLAYLIST:
			sorter = new PlaylistSorter;
			break;
		case CLIP_TYPE_TABLE:
			sorter = new TableSorter;
			break;
		case CLIP_TYPE_TICKER:
			sorter = new TickerSorter;
			break;
		case CLIP_TYPE_TEXT:
			sorter = new TextSorter;
			break;
		case CLIP_TYPE_TIMER:
			sorter = new TimerSorter;
			break;
		case CLIP_TYPE_WEATHER:
			sorter = new WeatherSorter;
			break;
	}

	if (sorter) {
		sorter->SetPlaylistClipsOnly(PlaylistClipsOnly());
		sorter->SetNameContainsFilterString(_NameContainsFilterString());
	}

	return sorter;
}

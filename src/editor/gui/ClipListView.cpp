/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipListView.h"

#include <new>
#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <ListItem.h>
#include <Message.h>
#include <Mime.h>
#include <ScrollBar.h>
#include <Window.h>

#include "Clip.h"
#include "ClipPlaylistItem.h"
#include "CommonPropertyIDs.h"
#include "Playlist.h"
#include "Selection.h"
#include "TimelineMessages.h"

using std::nothrow;

#define TEXT_OFFSET		6.0
#define BORDER_SPACING	4.0
#define ICON_SIZE		22.0

// constructor
ClipListItem::ClipListItem(Clip* c, ClipListView* listView)
	: SimpleItem(""),
	  clip(NULL),
	  fListView(listView),
	  fPainter(NULL),
	  fRemoved(false),
	  fReloadToken(0)
{
	SetClip(c);
}

// constructor
ClipListItem::ClipListItem(Clip* c, ClipListView* listView,
						   ClipItemPainter* painter)
	: SimpleItem(""),
	  clip(NULL),
	  fListView(listView),
	  fPainter(painter)
{
	SetClip(c);
}

// destructor
ClipListItem::~ClipListItem()
{
	SetClip(NULL);
	delete fPainter;
}

// Draw
void
ClipListItem::Draw(BView* owner, BRect frame, uint32 flags)
{
	if (fPainter) {
		SimpleItem::DrawBackground(owner, frame, flags);
		// additional graphics to distinguish clips
		fPainter->PaintItem(owner, this, frame, flags);
	} else {
		SimpleItem::Draw(owner, frame, flags);
	}
}

// Update
void
ClipListItem::Update(BView* owner, const BFont* font)
{
	SetWidth(owner->Bounds().Width());

	font_height fh;
	font->GetHeight(&fh);
	SetHeight(ceilf(max_c(fh.ascent + fh.descent, ICON_SIZE) + 6));
}

// ObjectChanged
void
ClipListItem::ObjectChanged(const Observable* object)
{
	if (object != clip)
		return;

	if (clip->Name() != Text())
		UpdateText();

	SetRemoved(clip->HasRemovedStatus());

	if (fReloadToken != clip->ChangeToken()) {
		fReloadToken = clip->ChangeToken();
		if (fPainter)
			fPainter->MakeIcon(clip);
		Invalidate();
	}
}

// SetClip
void
ClipListItem::SetClip(Clip* c)
{
	if (c == clip)
		return;

	if (clip) {
		clip->RemoveObserver(this);
		clip->Release();
	}

	clip = c;

	if (clip) {
		clip->Acquire();
		clip->AddObserver(this);

		UpdateText();
		SetRemoved(clip->HasRemovedStatus());
		fReloadToken = clip->ChangeToken();
	}
}

// SetPainter
void
ClipListItem::SetPainter(ClipItemPainter* painter)
{
	delete fPainter;
	fPainter = painter;
}

// UpdateText
void
ClipListItem::UpdateText()
{
	BString name = clip->Name();
	SetText(name.String());

	Invalidate();
}

// SetRemoved
void
ClipListItem::SetRemoved(bool removed)
{
	if (fRemoved == removed)
		return;

	fRemoved = removed;
	
	Invalidate();
}

// Invalidate
void
ClipListItem::Invalidate()
{
	// :-/
	if (fListView->LockLooper()) {
		fListView->InvalidateItem(
			fListView->IndexOf(this));
		fListView->UnlockLooper();
	}
}

// #pragma mark - ClipItemPainter

// constructor
ClipItemPainter::ClipItemPainter()
	: fIcon(new (nothrow) BBitmap(BRect(0, 0, ICON_SIZE - 1, ICON_SIZE - 1),
	  	0, B_RGBA32))
{
}

// destructor
ClipItemPainter::~ClipItemPainter()
{
	delete fIcon;
}

// MakeIcon
void
ClipItemPainter::MakeIcon(Clip* clip)
{
	if (fIcon != NULL && !clip->GetIcon(fIcon)) {
		// TODO: prettier empty icon
		memset((uint8*)fIcon->Bits(), 220, fIcon->BitsLength());
	}
}

// PaintItem
void
ClipItemPainter::PaintItem(BView* owner, ClipListItem* item,
						   BRect itemFrame, uint32 flags)
{
	owner->PushState();

	BFont font;
	owner->GetFont(&font);

	// configure font and color according to "removed" status
	if (item->Removed()) {
		font.SetShear(105.0);
		font.SetSpacing(B_STRING_SPACING);
		owner->SetFont(&font, B_FONT_SHEAR | B_FONT_SPACING);
		owner->SetHighColor(128, 128, 128, 255);
	} else {
		owner->SetHighColor(0, 0, 0, 255);
	}

	// truncate text to fit
	const char* text = item->Text();
	BString truncatedString(text);
	owner->TruncateString(&truncatedString, B_TRUNCATE_MIDDLE,
						  itemFrame.Width()
						  - BORDER_SPACING
						  - ICON_SIZE
						  - TEXT_OFFSET
						  - BORDER_SPACING);

	// figure out text position
	font_height fh;
	font.GetHeight(&fh);

	float height = itemFrame.Height();
	float textHeight = fh.ascent + fh.descent;
	BPoint pos;
	pos.x = itemFrame.left
			+ BORDER_SPACING + ICON_SIZE + TEXT_OFFSET;
	pos.y = itemFrame.top
			+ ceilf(height / 2.0 - textHeight / 2.0
				  		  + fh.ascent);

	// draw text
	owner->DrawString(truncatedString.String(), pos);

	// draw icon
	pos.x = itemFrame.left + BORDER_SPACING;
	pos.y = floorf((itemFrame.top + itemFrame.bottom - ICON_SIZE) / 2);
	if (fIcon != NULL) {
		owner->DrawBitmap(fIcon, pos);

		// stroke frame around icon
		BRect r = fIcon->Bounds();
		r.OffsetTo(pos);
		r.InsetBy(-1, -1);
		owner->SetHighColor(tint_color(owner->LowColor(), B_DARKEN_2_TINT));
		owner->StrokeRect(r);
	}

	owner->PopState();
}

// #pragma mark - ItemSorter

// constructor
ClipListView::ItemSorter::ItemSorter()
	: fPlaylistClipsOnly(false)
	, fNameContainsFilterString("")
{
}

// destructor
ClipListView::ItemSorter::~ItemSorter()
{
}

// SetPlaylistClipsOnly
void
ClipListView::ItemSorter::SetPlaylistClipsOnly(bool playlistOnly)
{
	fPlaylistClipsOnly = playlistOnly;
}

// SetNameContainsFilterString
void
ClipListView::ItemSorter::SetNameContainsFilterString(const char* string)
{
	fNameContainsFilterString = string;
}

// IndexForClip
int32
ClipListView::ItemSorter::IndexForClip(ClipListView* listView,
									   Clip* clip) const
{
	if (fNameContainsFilterString.Length() > 0
		&& clip->Name().IFindFirst(fNameContainsFilterString) < 0) {
		return -1;
	}

	::Playlist* playlist = listView->Playlist();
	if (fPlaylistClipsOnly && playlist) {
		// TODO: this test isn't quite what we want, we actually
		// want to assign clips to playlist, independent of wether
		// they are already used in the playlist, because otherwise,
		// newly created clips might not even show up in the list
		// at all, and the user cannot drag them into the playlist
		// (at which point they would show up)
		bool usesClip = false;
		int32 count = playlist->CountItems();
		for (int32 i = 0; i < count; i++) {
			PlaylistItem* item = playlist->ItemAtFast(i);
			if (ClipPlaylistItem* clipItem
					= dynamic_cast<ClipPlaylistItem*>(item)) {
				if (clipItem->Clip() == clip) {
					usesClip = true;
					break;
				}
			}
		}
		if (!usesClip)
			return -1;
	}

	// binary search
	int32 lower = 0;
	int32 upper = listView->CountItems();
	while (lower < upper) {
		int32 mid = (lower + upper) / 2;
		ClipListItem* item = dynamic_cast<ClipListItem*>(listView->ItemAt(mid));
		if (strcasecmp(item->Text(), clip->Name().String()) > 0)
			upper = mid;
		else
			lower = mid + 1;
	}
	return lower;
}

// #pragma mark - TemporaryIncrementer

class TemporaryIncrementer {
public:
	TemporaryIncrementer(int32& value)
		: fValue(value)
	{
		fValue++;
	}
	~TemporaryIncrementer()
	{
		fValue--;
	}
	int32& fValue;
};

// #pragma mark - ClipListView

// constructor
ClipListView::ClipListView(const char* name,
						   BMessage* selectionChangedMessage,
						   BMessage* invokeMessage,
						   BHandler* target)
	: SimpleListView(BRect(0.0, 0.0, 20.0, 20.0), name,
					 NULL, B_MULTIPLE_SELECTION_LIST),
	  fSelectionMessage(selectionChangedMessage),
	  fInvokeMessage(invokeMessage),
	  fClipLibrary(NULL),
	  fSelection(NULL),
	  fPlaylist(NULL),
	  fSorter(new (nothrow) ItemSorter()),
	  fIgnoreSelectionChanged(0)
{
	SetDragCommand(MSG_DRAG_CLIP);
	SetTarget(target);
}

// destructor
ClipListView::~ClipListView()
{
	_MakeEmpty();
	delete fSelectionMessage;
	delete fInvokeMessage;

	if (fClipLibrary)
		fClipLibrary->RemoveListener(this);
	SetPlaylist(NULL);
	SetSelection(NULL);

	delete fSorter;
}

// SelectionChanged
void
ClipListView::SelectionChanged()
{
	// NOTE: this is a single selection list!

	if (fIgnoreSelectionChanged > 0)
		return;

	ClipListItem* item = dynamic_cast<ClipListItem*>(ItemAt(CurrentSelection(0)));
	if (fSelectionMessage) {
		BMessage message(*fSelectionMessage);
		message.AddPointer("clip", item ? (void*)item->clip : NULL);
		Invoke(&message);
	}

	// modify global Selection
	if (fSelection == NULL)
		return;

	if (item)
		fSelection->Select(item->clip);
	else
		fSelection->DeselectAll();
}

// MessageReceived
void
ClipListView::MessageReceived(BMessage* message)
{
	if (message->what == B_SIMPLE_DATA) {
		// drag and drop, most likely from Tracker
		be_app->PostMessage(message);
	} else {
		if (message->what != MSG_DRAG_CLIP)
			// don't allow drag sorting for now
			SimpleListView::MessageReceived(message);
		else
			BListView::MessageReceived(message);
	}
}

// MakeDragMessage
void
ClipListView::MakeDragMessage(BMessage* message) const
{
	SimpleListView::MakeDragMessage(message);
	message->AddPointer("library", fClipLibrary);
	int32 count = CountItems();
	for (int32 i = 0; i < count; i++) {
		ClipListItem* item = dynamic_cast<ClipListItem*>(ItemAt(CurrentSelection(i)));
		if (item) {
			message->AddPointer("clip", (void*)item->clip);
		} else
			break;
	}

//		message->AddInt32("be:actions", B_COPY_TARGET);
//		message->AddInt32("be:actions", B_TRASH_TARGET);
//
//		message->AddString("be:types", B_FILE_MIME_TYPE);
////		message->AddString("be:filetypes", "");
////		message->AddString("be:type_descriptions", "");
//
//		message->AddString("be:clip_name", item->clip->Name());
//
//		message->AddString("be:originator", "Clockwerk");
//		message->AddPointer("be:originator_data", (void*)item->clip);
}

// AcceptDragMessage
bool
ClipListView::AcceptDragMessage(const BMessage* message) const
{
	if (message->what == B_SIMPLE_DATA)
		return true;
//	return SimpleListView::AcceptDragMessage(message);
	// don't allow drag sorting for now
	return false;
}

// SetDropTargetRect
void
ClipListView::SetDropTargetRect(const BMessage* message, BPoint where)
{
	if (message->what == B_SIMPLE_DATA) {
		_SetDropAnticipationRect(Bounds());
	} else {
		SimpleListView::SetDropTargetRect(message, where);
	}
}

// MoveItems
void
ClipListView::MoveItems(BList& items, int32 toIndex)
{
	SimpleListView::MoveItems(items, toIndex);
}

// CopyItems
void
ClipListView::CopyItems(BList& items, int32 toIndex)
{
	MoveItems(items, toIndex);
	// copy operation not allowed -> ?!?
	// TODO: what about clips that reference the same file but
	// with different "in/out points"
}

// RemoveItemList
void
ClipListView::RemoveItemList(BList& indices)
{
	// not allowed
}

// CloneItem
BListItem*
ClipListView::CloneItem(int32 index) const
{
	if (ClipListItem* item = dynamic_cast<ClipListItem*>(ItemAt(index))) {
		return new (nothrow) ClipListItem(item->clip,
			const_cast<ClipListView*>(this));
	}
	return NULL;
}

// DoubleClicked
void
ClipListView::DoubleClicked(int32 index)
{
	if (!fInvokeMessage)
		return;

	ClipListItem* item = dynamic_cast<ClipListItem*>(ItemAt(index));
	if (!item || !item->clip)
		return;

	BMessage message(*fInvokeMessage);
	message.AddPointer("clip", (void*)item->clip);
	Invoke(&message);
}


// #pragma mark -

// ObjectAdded
void
ClipListView::ObjectAdded(ServerObject* object, int32 index)
{
	// NOTE: we're only interested in Clip objects
	Clip* clip = dynamic_cast<Clip*>(object);
	if (!clip)
		return;

	// NOTE: we are in the thread that messed with the
	// ServerObjectManager, so no need to lock the
	// manager, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	index = _FilterClip(clip);
	if (index >= 0)
		_AddClip(clip, index);

	UnlockLooper();
}

// ObjectRemoved
void
ClipListView::ObjectRemoved(ServerObject* object)
{
	// NOTE: we are in the thread that messed with the
	// ServerObjectManager, so no need to lock the
	// manager, when this is changed to asynchronous
	// notifications, then it would need to be read-locked!
	if (!LockLooper())
		return;

	// NOTE: we're only interested in Clip objects
	_RemoveClip(dynamic_cast<Clip*>(object));

	UnlockLooper();
}

// #pragma mark -

// ItemAdded
void
ClipListView::ItemAdded(::Playlist* playlist, PlaylistItem* item, int32 index)
{
	if (fSorter->PlaylistClipsOnly())
		_Sync();
}

// ItemRemoved
void
ClipListView::ItemRemoved(::Playlist* playlist, PlaylistItem* item)
{
	if (fSorter->PlaylistClipsOnly())
		_Sync();
}

// #pragma mark -

// ObjectChanged
void
ClipListView::ObjectChanged(const Observable* object)
{
	if (object == fSelection) {
		TemporaryIncrementer _(fIgnoreSelectionChanged);

		Selectable* selectable = fSelection->SelectableAt(0);
		Clip* clip = dynamic_cast<Clip*>(selectable);
		if (clip) {
			Select(IndexForClip(clip));
		} else
			DeselectAll();
	}
}

// #pragma mark -

// SetObjectLibrary
void
ClipListView::SetObjectLibrary(ServerObjectManager* library)
{
	if (fClipLibrary == library)
		return;

	// detach from old library
	if (fClipLibrary)
		fClipLibrary->RemoveListener(this);

	_MakeEmpty();

	fClipLibrary = library;

	if (fClipLibrary == NULL)
		return;

	fClipLibrary->AddListener(this);

	_Sync();
}

// SetSelection
void
ClipListView::SetSelection(Selection* selection)
{
	if (fSelection == selection)
		return;

	if (fSelection)
		fSelection->RemoveObserver(this);

	fSelection = selection;

	if (fSelection)
		fSelection->AddObserver(this);
}

// SetPlaylist
void
ClipListView::SetPlaylist(::Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	if (fPlaylist) {
		fPlaylist->RemoveListObserver(this);
		fPlaylist->Release();
	}

	fPlaylist = playlist;

	if (fPlaylist) {
		fPlaylist->Acquire();
		fPlaylist->AddListObserver(this);
	}

	if (fSorter->PlaylistClipsOnly())
		_Sync();
}

// SetItemSorter
void
ClipListView::SetItemSorter(ItemSorter* sorter)
{
	delete fSorter;
	if (sorter)
		fSorter = sorter;
	else
		fSorter = new (nothrow) ItemSorter();

	_Sync();
}

// NameContainsFilterString
const BString&
ClipListView::NameContainsFilterString() const
{
	if (fSorter)
		return fSorter->NameContainsFilterString();

	static const BString emptyString("");
	return emptyString;
}

// IndexForClip
int32
ClipListView::IndexForClip(Clip* clip) const
{
	for (int32 i = 0; ClipListItem* item
			= dynamic_cast<ClipListItem*>(ItemAt(i)); i++) {
		if (item->clip == clip)
			return i;
	}
	return -1;
}

// #pragma mark -

// _AddClip
bool
ClipListView::_AddClip(Clip* clip, int32 index)
{
	if (!clip)
		return false;

	ClipItemPainter* painter = new (nothrow) ClipItemPainter();
	if (!painter)
		return false;
	painter->MakeIcon(clip);

	ClipListItem* item = new (nothrow) ClipListItem(clip, this, painter);
	if (!item || !AddItem(item, index)) {
		delete item;
		return false;
	}

	if (clip->IsSelected()) {
		TemporaryIncrementer _(fIgnoreSelectionChanged);
		Select(index, true);
	}

	return true;
}

// _RemoveClip
bool
ClipListView::_RemoveClip(Clip* clip)
{
	ClipListItem* item = _ItemForClip(clip);
	if (item && RemoveItem(item)) {
		delete item;
		return true;
	}
	return false;
}

// _ItemForClip
ClipListItem*
ClipListView::_ItemForClip(Clip* clip) const
{
	return dynamic_cast<ClipListItem*>(ItemAt(IndexForClip(clip)));
}

// _MakeEmpty
void
ClipListView::_MakeEmpty()
{
// Begin hack to make list modification faster
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar) {
		if (Window())
			Window()->UpdateIfNeeded();
		scrollBar->SetTarget((BView*)NULL);
	}
// End hack to make list modification faster

	// NOTE: BListView::MakeEmpty() uses ScrollTo()
	// for which the object needs to be attached to
	// a BWindow.... :-(
	int32 count = CountItems();
	for (int32 i = count - 1; i >= 0; i--) {
		delete RemoveItem(i);
	}

// Begin hack to make list modification faster
	if (scrollBar) {
		scrollBar->SetTarget(this);
		BListView::FrameResized(Bounds().Width(), Bounds().Height());
	}
// End hack to make list modification faster
}

// _Sync
void
ClipListView::_Sync()
{
	if (!fClipLibrary)
		return;

// Begin hack to make list modification faster
	BScrollBar* scrollBar = ScrollBar(B_VERTICAL);
	if (scrollBar) {
		if (Window())
			Window()->UpdateIfNeeded();
		scrollBar->SetTarget((BView*)NULL);
	}
// End hack to make list modification faster

	TemporaryIncrementer _(fIgnoreSelectionChanged);

	_MakeEmpty();

	// sync
	if (fClipLibrary->WriteLock()) {
		int32 count = fClipLibrary->CountObjects();
		for (int32 i = 0; i < count; i++) {
			// NOTE: we are only interested in Clip objects
			Clip* clip = dynamic_cast<Clip*>(fClipLibrary->ObjectAtFast(i));
			int32 index = _FilterClip(clip);
			if (index >= 0) {
				_AddClip(clip, index);
			}
		}
	
		fClipLibrary->WriteUnlock();
	}

// Begin hack to make list modification faster
	if (scrollBar) {
		scrollBar->SetTarget(this);
		BListView::FrameResized(Bounds().Width(), Bounds().Height());
	}
// End hack to make list modification faster
}

// _FilterClip
int32
ClipListView::_FilterClip(Clip* clip)
{
	if (!clip)
		return -1;

	if (fSorter)
		return fSorter->IndexForClip(this, clip);

	return CountItems();
}


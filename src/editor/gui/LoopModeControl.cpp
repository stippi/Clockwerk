/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "LoopModeControl.h"

#include <stdio.h>

#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>

#include "LoopMode.h"
#include "PlaybackManager.h"

enum {
	MSG_SET_LOOP_MODE		= 'stlm',
};

// constructor
LoopModeControl::LoopModeControl()
	: BMenuField(BRect(0, 0, 150, 15), "loop mode control", "Playback",
				 _CreateMenu(), true, B_FOLLOW_NONE)
	, fLoopMode(NULL)
	, fLoopModeCache(LOOPING_ALL)
{
	float labelWidth = StringWidth(Label()) + 5;
	SetDivider(labelWidth);
	float width;
	float height;
	MenuBar()->GetPreferredSize(&width, &height);
	width = max_c(width, MenuBar()->Frame().Width());
	ResizeTo(width + 4 + labelWidth, height + 6);
	MenuBar()->ResizeTo(width, height);
}

// destructor
LoopModeControl::~LoopModeControl()
{
	SetLoopMode(NULL);
}

// AttachedToWindow
void
LoopModeControl::AttachedToWindow()
{
	BMenuField::AttachedToWindow();
	Menu()->SetTargetForItems(this);
}

// MessageReceived
void
LoopModeControl::MessageReceived(BMessage* message)
{
	switch (message->what) {

		case MSG_SET_LOOP_MODE:
			if (fLoopMode) {
				int32 mode;
				if (message->FindInt32("mode", &mode) == B_OK) {
					fLoopModeCache = mode;
						// prevent updating the menu item, because
						// it is already current by adjusting fLoopModeCache
						// beforehand
					fLoopMode->SetMode(mode);
				}
			}
			break;

		default:
			BMenuField::MessageReceived(message);
			break;
	}
}

// ObjectChanged
void
LoopModeControl::ObjectChanged(const Observable* object)
{
	if (!LockLooper())
		return;

	if (object == fLoopMode) {
		if (fLoopModeCache != fLoopMode->Mode()) {
			fLoopModeCache = fLoopMode->Mode();
			switch (fLoopModeCache) {
				case LOOPING_ALL:
					fAllItem->SetMarked(true);
					break;
				case LOOPING_RANGE:
					fRangeItem->SetMarked(true);
					break;
				case LOOPING_VISIBLE:
					fVisibleItem->SetMarked(true);
					break;
			}
		}
	}

	UnlockLooper();
}

// SetLoopMode
void
LoopModeControl::SetLoopMode(LoopMode* mode)
{
	if (fLoopMode == mode)
		return;

	if (fLoopMode)
		fLoopMode->RemoveObserver(this);

	fLoopMode = mode;

	if (fLoopMode)
		fLoopMode->AddObserver(this);
}

// #pragma mark -

// _CreateMenu
BMenu*
LoopModeControl::_CreateMenu()
{
	BPopUpMenu* menu = new BPopUpMenu("loop mode");

	BMessage* message = new BMessage(MSG_SET_LOOP_MODE);
	message->AddInt32("mode", LOOPING_ALL);
	fAllItem = new BMenuItem("All", message);
	menu->AddItem(fAllItem);
	fAllItem->SetMarked(true);

	message = new BMessage(MSG_SET_LOOP_MODE);
	message->AddInt32("mode", LOOPING_RANGE);
	fRangeItem = new BMenuItem("Range", message);
	menu->AddItem(fRangeItem);

	message = new BMessage(MSG_SET_LOOP_MODE);
	message->AddInt32("mode", LOOPING_VISIBLE);
	fVisibleItem = new BMenuItem("Visible", message);
	menu->AddItem(fVisibleItem);

	return menu;
}

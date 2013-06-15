/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SlideShowWindow.h"

#include <new>
#include <stdio.h>

#include <Box.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <TextControl.h>

#include "CommandStack.h"
#include "CommonPropertyIDs.h"
#include "EditorApp.h"
#include "PlaylistListView.h"
#include "RWLocker.h"
#include "SlideShowPlaylist.h"

using std::nothrow;

enum {
	MSG_SET_TOTAL_DURATION			= 'stod',
	MSG_SET_TRANSITION_DURATION		= 'strd',
};

// constructor
SlideShowWindow::SlideShowWindow(BRect frame, RWLocker* locker)
	:
	BWindow(frame, "Slide Show Playlist", B_FLOATING_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS
			| B_AUTO_UPDATE_SIZE_LIMITS),
	  fPlaylist(NULL),
	  fLocker(locker),
	  fCommandStack(new (nothrow) CommandStack())
{
//	fCommandStack->SetLocker(fLocker);
	_Init();
}

// destructor
SlideShowWindow::~SlideShowWindow()
{
	SetPlaylist(NULL);

	delete fCommandStack;
}

// #pragma mark -

// MessageReceived
void
SlideShowWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREPARE_FOR_SYNCHRONIZATION:
			SetPlaylist(NULL);
			PostMessage(B_QUIT_REQUESTED);

			message->SendReply(MSG_READY_FOR_SYNCHRONIZATION);
			break;

		case MSG_SET_TOTAL_DURATION: {
			AutoWriteLocker _(fLocker);
			fPlaylist->SetValue(PROPERTY_DURATION, fTotalDuration->Text());
			break;
		}
		case MSG_SET_TRANSITION_DURATION: {
			AutoWriteLocker _(fLocker);
			fPlaylist->SetValue(PROPERTY_TRANSITION_DURATION,
								fTransitionDuration->Text());
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
SlideShowWindow::QuitRequested()
{
	return true;
}

// #pragma mark -

// ObjectChanged
void
SlideShowWindow::ObjectChanged(const Observable* object)
{
	if (!Lock())
		return;

	AutoWriteLocker _(fLocker);

	_UpdateControls();
	fPlaylist->ValidateItemLayout();

	Unlock();
}

// NotificationBlockFinished
void
SlideShowWindow::NotificationBlockFinished(Playlist* playlist)
{
	fPlaylist->ValidateItemLayout();
}

// #pragma mark -

// SetPlaylist
void
SlideShowWindow::SetPlaylist(SlideShowPlaylist* playlist)
{
	if (fPlaylist == playlist)
		return;

	fListView->SetPlaylist(playlist);

	if (fPlaylist) {
		fPlaylist->RemoveListObserver(this);
		fPlaylist->RemoveObserver(this);

		if (!fCommandStack->IsSaved())
			fPlaylist->SetDataSaved(false);

		fPlaylist->Release();
	}

	fPlaylist = playlist;

	if (fPlaylist) {
		fPlaylist->Acquire();
		fPlaylist->AddObserver(this);
		fPlaylist->AddListObserver(this);
	}

	_UpdateControls();
}

// #pragma mark -

// _Init
void
SlideShowWindow::_Init()
{
	// create the GUI
	_CreateGUI();

	// take some additional care of the scroll bar that scrolls
	// the list view
	if (BScrollBar* scrollBar = fListView->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 2);
	}

	fListView->SetCommandStack(fCommandStack);

	_UpdateControls();
}

// _CreateGUI
void
SlideShowWindow::_CreateGUI()
{
	fMenuBar = _CreateMenuBar();

	// clip list view
	fListView = new PlaylistListView(fLocker, "auto playlist listview");

	// scroll view around list view
	BScrollView* listScrollView = new BScrollView("list scroll view",
		fListView, 0, false, true, B_NO_BORDER);

	// total duration
	fTotalDuration = new BTextControl("total duration", "Total Duration", "",
		new BMessage(MSG_SET_TOTAL_DURATION));

	// transition duration
	fTransitionDuration = new BTextControl("transition duration",
		"Transition Duration", "", new BMessage(MSG_SET_TRANSITION_DURATION));

	// create container for all other views
	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);
	BGroupLayoutBuilder(layout)
		.Add(fMenuBar)
		.AddGroup(B_HORIZONTAL, 5.0f)
			.Add(listScrollView)
			.AddGroup(B_VERTICAL)
				.Add(BGridLayoutBuilder(5.0f, 5.0f)
					.Add(fTotalDuration->CreateLabelLayoutItem(), 0, 0)
					.Add(fTotalDuration->CreateTextViewLayoutItem(), 1, 0)
					.Add(fTransitionDuration->CreateLabelLayoutItem(), 0, 1)
					.Add(fTransitionDuration->CreateTextViewLayoutItem(), 1, 1)
					.SetInsets(5.0f, 5.0f, 5.0f, 5.0f)
				)
				.AddGlue()
			.End()
		.End()
	;
}

// _CreateMenuBar
BMenuBar*
SlideShowWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("menu bar");

	// Client Settings
	fMenu = new BMenu("Dummy");

	menuBar->AddItem(fMenu);

	return menuBar;
}

// _UpdateControls
void
SlideShowWindow::_UpdateControls()
{
	if (fPlaylist) {
		BString duration;
		if (fPlaylist->GetValue(PROPERTY_DURATION, duration)) {
			fTotalDuration->SetText(duration.String());
			fTotalDuration->SetEnabled(true);
		}
		duration = "";
		if (fPlaylist->GetValue(PROPERTY_TRANSITION_DURATION, duration)) {
			fTransitionDuration->SetText(duration.String());
			fTransitionDuration->SetEnabled(true);
		}
	} else {
		fTotalDuration->SetText("");
		fTotalDuration->SetEnabled(false);

		fTransitionDuration->SetText("");
		fTransitionDuration->SetEnabled(false);
	}
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "UserWindow.h"

#include <new>
#include <stdio.h>

#include <Box.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <TextControl.h>

#include "EditorApp.h"
#include "CommandStack.h"
#include "CommonPropertyIDs.h"
#include "Group.h"
#include "MessageConstants.h"
#include "ScrollView.h"
#include "ServerObject.h"
#include "UserListView.h"

using std::nothrow;

enum {
	MSG_CREATE_USER				= 'crus',

	MSG_USER_SELECTED			= 'ussl',

	MSG_SET_NAME				= 'stnm',
	MSG_SET_PASSWORD			= 'stpw',
};

// constructor
UserWindow::UserWindow(BRect frame, EditorApp* app)
	: BWindow(frame, "Users",
			  B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
			  B_ASYNCHRONOUS_CONTROLS),
	  fApp(app),
	  fObjectManager(NULL),
	  fCurrentUser(NULL)
{
	_Init();
}

// destructor
UserWindow::~UserWindow()
{
	if (fObjectManager)
		fObjectManager->RemoveListener(this);

	SetCurrentUser(NULL);
}

// #pragma mark -

// MessageReceived
void
UserWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREPARE_FOR_SYNCHRONIZATION:
			SetCurrentUser(NULL);
			fListView->DeselectAll();
			message->SendReply(MSG_READY_FOR_SYNCHRONIZATION);
			break;

		case MSG_CREATE_USER:
			if (fObjectManager) {
				ServerObject* o = new (nothrow) ServerObject("User");
				o->SetName("<new user>");
				fObjectManager->AddObject(o);
			}
			break;

		case MSG_USER_SELECTED: {
			ServerObject* object;
			if (message->FindPointer("object", (void**)&object) < B_OK)
				object = NULL;
			SetCurrentUser(object);
			break;
		}

		case MSG_SET_NAME:
			if (fCurrentUser)
				fCurrentUser->SetName(fUserName->Text());
			break;

		case MSG_SET_PASSWORD:
			if (fCurrentUser)
				fCurrentUser->SetValue(PROPERTY_PASSWORD, fPassword->Text());
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
UserWindow::QuitRequested()
{
	Hide();

	return false;
}

// #pragma mark -

// ObjectChanged
void
UserWindow::ObjectChanged(const Observable* object)
{
	if (!Lock())
		return;

	// TODO: ?

	Unlock();
}

// ObjectAdded
void
UserWindow::ObjectAdded(ServerObject* object, int32 index)
{
	// nothing to do
}

// ObjectRemoved
void
UserWindow::ObjectRemoved(ServerObject* object)
{
	if (object == fCurrentUser)
		SetCurrentUser(NULL);
}

// #pragma mark -

// SetObjectManager
void
UserWindow::SetObjectManager(ServerObjectManager* manager)
{
	if (fObjectManager == manager)
		return;

	if (fObjectManager)
		fObjectManager->RemoveListener(this);

	fObjectManager = manager;

	// hook up the list view to clip library
	fListView->SetObjectLibrary(fObjectManager);

	if (fObjectManager)
		fObjectManager->AddListener(this);
}

// SetCurrentUser
void
UserWindow::SetCurrentUser(ServerObject* object)
{
	if (fCurrentUser == object)
		return;

	if (!fObjectManager->ReadLock()) {
printf("unable to readlock object library\n");
		return;
	}

	if (fCurrentUser) {
		fCurrentUser->RemoveObserver(this);
		fCurrentUser->Release();
	}

	fCurrentUser = object;

	if (fCurrentUser) {
		fCurrentUser->Acquire();
		fCurrentUser->AddObserver(this);
	}

	_UpdateUser();

	fObjectManager->ReadUnlock();
}

// #pragma mark -

// _Init
void
UserWindow::_Init()
{
	// create the GUI
	_CreateGUI(Bounds());

	// take some additional care of the scroll bar that scrolls
	// the list view
	if (BScrollBar* scrollBar = fListView->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 2);
	}

	// set some arbitrary size limits (TODO: real layout stuff)
//	float minWidth, maxWidth, minHeight, maxHeight;
//	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
//	minWidth = 620;
//	minHeight = 400;
//	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	_UpdateUser();
}

// _CreateGUI
void
UserWindow::_CreateGUI(BRect bounds)
{
	// TODO: make simple layout framework

	// create container for all other views
	BView* bg = new BView(bounds, "background view", B_FOLLOW_ALL, 0);
	bg->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	AddChild(bg);

	BRect menuFrame(bounds);
	fMenuBar = _CreateMenuBar(menuFrame);
	bg->AddChild(fMenuBar);
	fMenuBar->ResizeToPreferred();
	fMenuBar->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	// clip list view
	fListView = new UserListView("user listview",
								 new BMessage(MSG_USER_SELECTED), this);
//	fListView->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	// scroll view around list view
	bounds.top = fMenuBar->Frame().bottom + 1;
	bounds.right = bounds.left + 120;
	fListView->MoveTo(bounds.LeftTop());
	fListView->ResizeTo(bounds.Width(), bounds.Height());

	bg->AddChild(new BScrollView("list scroll view",
								 fListView,
								 B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM,
								 0, false, true,
								 B_NO_BORDER));

	// username
	bounds.top += 5;
	bounds.left += bounds.right + B_V_SCROLL_BAR_WIDTH + 5;
	bounds.right = bg->Bounds().right - 5;
	bounds.bottom = bounds.top + 15;
	fUserName = new BTextControl(bounds, "user name",
								 "Username", "",
								 new BMessage(MSG_SET_NAME),
								 B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	float width, height;
	fUserName->GetPreferredSize(&width, &height);
	fUserName->ResizeTo(bounds.Width(), height);
		// can't use ResizeToPreferred() because it shrinks the width :-/

	bg->AddChild(fUserName);

	// password
	bounds.OffsetBy(0, fUserName->Frame().Height() + 6);

	fPassword = new BTextControl(bounds, "password",
								 "Password", "",
								 new BMessage(MSG_SET_PASSWORD),
								 B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	fPassword->GetPreferredSize(&width, &height);
	fPassword->ResizeTo(bounds.Width(), height);
		// can't use ResizeToPreferred() because it shrinks the width :-/

	bg->AddChild(fPassword);


	// align dividers
	float maxLabel = fUserName->StringWidth(fUserName->Label());
	maxLabel = max_c(fPassword->StringWidth(fPassword->Label()),
					 maxLabel);
	maxLabel += 8;
	fUserName->SetDivider(maxLabel);
	fPassword->SetDivider(maxLabel);
}

// _CreateMenuBar
BMenuBar*
UserWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "menu bar");

	// Client Settings
	fUserMenu = new BMenu("User");
	BMenuItem* item = new BMenuItem("New", new BMessage(MSG_CREATE_USER), 'N');
	fUserMenu->AddItem(item);
	fUserMenu->SetTargetForItems(this);

	menuBar->AddItem(fUserMenu);

	return menuBar;
}

// _UpdateUser
void
UserWindow::_UpdateUser()
{
	if (fCurrentUser) {
		fUserName->SetText(fCurrentUser->Name().String());
		fUserName->SetEnabled(true);

		BString password;
		fCurrentUser->GetValue(PROPERTY_PASSWORD, password);
		fPassword->SetText(password.String());
		fPassword->SetEnabled(true);
	} else {
		fUserName->SetText("select a user from the list");
		fUserName->SetEnabled(false);

		fPassword->SetEnabled(false);
		fPassword->SetText("");
	}
}




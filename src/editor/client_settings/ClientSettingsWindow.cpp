/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClientSettingsWindow.h"

#include <new>
#include <stdio.h>

#include <Box.h>
#include <CheckBox.h>
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

#include "EditorApp.h"
#include "ClientSettingsListView.h"
#include "CommandStack.h"
#include "CommonPropertyIDs.h"
#include "Group.h"
#include "MessageConstants.h"
#include "Playlist.h"
#include "ScrollView.h"
#include "ServerObject.h"

using std::nothrow;

enum {
	MSG_CREATE_CLIENT_SETTINGS	= 'crcs',

	MSG_SETTINGS_SELECTED		= 'stsl',

	MSG_SET_NAME				= 'stnm',
	MSG_SET_SCOPE				= 'stsc',

	MSG_SET_SERVER_ADDRESS_1	= 'sts1',
	MSG_SET_SERVER_ADDRESS_2	= 'sts2',
	MSG_SET_ERASE_OBJECTS		= 'steo',
	MSG_SET_ERASE_REVISIONS		= 'ster',
	MSG_SET_LOG_UPLOAD_TIME		= 'stlt',
};

// constructor
ClientSettingsWindow::ClientSettingsWindow(BRect frame, EditorApp* app)
	:
	BWindow(frame, "Client Settings", B_FLOATING_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fApp(app),
	fObjectManager(NULL),
	fCurrentSettings(NULL)
{
	_Init();
}

// destructor
ClientSettingsWindow::~ClientSettingsWindow()
{
	if (fObjectManager)
		fObjectManager->RemoveListener(this);

	SetCurrentSettings(NULL);
}

// #pragma mark -

// MessageReceived
void
ClientSettingsWindow::MessageReceived(BMessage* message)
{
	AutoWriteLocker locker(fObjectManager ? fObjectManager->Locker() : NULL);
	if (!locker.IsLocked()) {
		BWindow::MessageReceived(message);
		return;
	}

	switch (message->what) {
		case MSG_PREPARE_FOR_SYNCHRONIZATION:
			SetCurrentSettings(NULL);
			fListView->DeselectAll();
			message->SendReply(MSG_READY_FOR_SYNCHRONIZATION);
			break;

		case MSG_CREATE_CLIENT_SETTINGS:
			if (fObjectManager && fObjectManager->WriteLock()) {
				ServerObject* o = new (nothrow) ServerObject("ClientSettings");
				o->SetName("<new settings>");
				fObjectManager->AddObject(o);
				fObjectManager->WriteUnlock();
			}
			break;

		case MSG_SETTINGS_SELECTED: {
			ServerObject* object;
			if (message->FindPointer("object", (void**)&object) == B_OK) {
				SetCurrentSettings(object);
			} else {
				SetCurrentSettings(NULL);
			}
			break;
		}

		case MSG_SET_NAME:
			if (fCurrentSettings)
				fCurrentSettings->SetName(fSettingsName->Text());
			break;

		case MSG_SET_SCOPE:
			if (fCurrentSettings) {
				BString scope;
				if (message->FindString("scop", &scope) < B_OK)
					break;
				fCurrentSettings->SetValue(PROPERTY_SCOPE, scope.String());
			}
			break;

		case MSG_SET_SERVER_ADDRESS_1:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_SERVER_IP,
					fServerAddress1->Text());
			}
			break;
		case MSG_SET_SERVER_ADDRESS_2:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_SECONDARY_SERVER_IP,
					fServerAddress2->Text());
			}
			break;
		case MSG_SET_ERASE_OBJECTS:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_ERASE_DATA_FOLDER,
					fEraseObjects->Value() == B_CONTROL_ON);
			}
			break;
		case MSG_SET_ERASE_REVISIONS:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_ERASE_OLD_REVISIONS,
					fEraseRevisions->Value() == B_CONTROL_ON);
			}
			break;
		case MSG_SET_LOG_UPLOAD_TIME:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_LOG_UPLOAD_TIME,
					fLogUploadTime->Text());
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
ClientSettingsWindow::QuitRequested()
{
	Hide();

	return false;
}

// #pragma mark -

// ObjectChanged
void
ClientSettingsWindow::ObjectChanged(const Observable* object)
{
	if (!Lock())
		return;

	// TODO: ?

	Unlock();
}

// ObjectAdded
void
ClientSettingsWindow::ObjectAdded(ServerObject* object, int32 index)
{
//	BMessage message(MSG_OBJECT_ADDED);
//	message.AddPointer("object", object);
//	message.AddInt32("index", index);
//	PostMessage(&message, this);
}

// ObjectRemoved
void
ClientSettingsWindow::ObjectRemoved(ServerObject* object)
{
//	BMessage message(MSG_OBJECT_REMOVED);
//	message.AddPointer("object", object);
//	PostMessage(&message, this);
	if (object == fCurrentSettings)
		SetCurrentSettings(NULL);
}

// #pragma mark -

// SetObjectManager
void
ClientSettingsWindow::SetObjectManager(ServerObjectManager* manager)
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

// SetCurrentSettings
void
ClientSettingsWindow::SetCurrentSettings(ServerObject* object)
{
	if (fCurrentSettings == object)
		return;

	if (!fObjectManager->ReadLock()) {
printf("unable to readlock object library\n");
		return;
	}

	if (fCurrentSettings) {
		fCurrentSettings->RemoveObserver(this);
		fCurrentSettings->Release();
	}

	fCurrentSettings = object;

	if (fCurrentSettings) {
		fCurrentSettings->Acquire();
		fCurrentSettings->AddObserver(this);
	}

	_UpdateSettings();

	fObjectManager->ReadUnlock();
}

// SetScopes
void
ClientSettingsWindow::SetScopes(const BMessage* _scopes)
{
	BMenu* menu = fSettingsScope->Menu();
	// clean out menu
	while (BMenuItem* item = menu->RemoveItem(0L))
		delete item;

	BMessage scopes(*_scopes);
	if (!scopes.HasString("scop"))
		scopes.AddString("scop", "all");

	BString scope;
	for (int32 i = 0; scopes.FindString("scop", i, &scope) == B_OK; i++) {
		BMessage* message = new BMessage(MSG_SET_SCOPE);
		message->AddString("scop", scope.String());

		BMenuItem* item = new BMenuItem(scope.String(), message);
		menu->AddItem(item);
		item->SetTarget(this);
	}
}

// #pragma mark -

// _Init
void
ClientSettingsWindow::_Init()
{
	// create the GUI
	_CreateGUI();

	// take some additional care of the scroll bar that scrolls
	// the list view
	if (BScrollBar* scrollBar = fListView->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 2);
	}

	// add default scope item
	BMessage scopes;
	SetScopes(&scopes);

	_UpdateSettings();
}

// _CreateGUI
void
ClientSettingsWindow::_CreateGUI()
{
	fMenuBar = _CreateMenuBar();

	// clip list view
	fListView = new ClientSettingsListView("client settings lv",
			new BMessage(MSG_SETTINGS_SELECTED), this);

	// scroll view around list view
	BScrollView* listScrollView = new BScrollView("list scroll view",
		fListView, 0, false, true, B_NO_BORDER);

	// client settings name
	fSettingsName = new BTextControl("settings name",
		"Settings Name", "", new BMessage(MSG_SET_NAME));

	// client scope(s)
	fSettingsScope = new BMenuField("scopes", "Settings Scope",
		new BPopUpMenu("all"), NULL);

	// primary server address
	fServerAddress1 = new BTextControl("server ip 1",
		"Server Address 1", "", new BMessage(MSG_SET_SERVER_ADDRESS_1));

	// secondary server address
	fServerAddress2 = new BTextControl("server ip 2",
		"Server Address 2", "", new BMessage(MSG_SET_SERVER_ADDRESS_2));

	// erase objects
	fEraseObjects = new BCheckBox("erase objects",
		"Erase no more needed objects", new BMessage(MSG_SET_ERASE_OBJECTS));

	// erase old revisions
	fEraseRevisions = new BCheckBox("erase revisions",
		"Erase old software revisions", new BMessage(MSG_SET_ERASE_REVISIONS));

	// log upload time
	fLogUploadTime = new BTextControl("log upload time",
		"Daily Log Upload Time", "", new BMessage(MSG_SET_LOG_UPLOAD_TIME));


	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fMenuBar)
		.AddGroup(B_HORIZONTAL)
			.Add(listScrollView)
			.Add(BGridLayoutBuilder(5.0f, 5.0f)
				.Add(fSettingsName->CreateLabelLayoutItem(), 0, 0)
				.Add(fSettingsName->CreateTextViewLayoutItem(), 1, 0)
				.Add(fSettingsScope->CreateLabelLayoutItem(), 0, 1)
				.Add(fSettingsScope->CreateMenuBarLayoutItem(), 1, 1)
				.Add(fServerAddress1->CreateLabelLayoutItem(), 0, 2)
				.Add(fServerAddress1->CreateTextViewLayoutItem(), 1, 2)
				.Add(fServerAddress2->CreateLabelLayoutItem(), 0, 3)
				.Add(fServerAddress2->CreateTextViewLayoutItem(), 1, 3)
				.Add(fEraseObjects, 0, 4, 2)
				.Add(fEraseRevisions, 0, 5, 2)
				.Add(fLogUploadTime->CreateLabelLayoutItem(), 0, 6)
				.Add(fLogUploadTime->CreateTextViewLayoutItem(), 1, 6)
				.SetInsets(5.0f, 5.0f, 5.0f, 5.0f)
			)
		.End()
	);
}

// _CreateMenuBar
BMenuBar*
ClientSettingsWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("menu bar");

	// Client Settings
	fClientSettingsMenu = new BMenu("Client Settings");
	BMenuItem* item = new BMenuItem("New",
		new BMessage(MSG_CREATE_CLIENT_SETTINGS), 'N');
	fClientSettingsMenu->AddItem(item);
	fClientSettingsMenu->SetTargetForItems(this);

	menuBar->AddItem(fClientSettingsMenu);

	return menuBar;
}

// _UpdateSettings
void
ClientSettingsWindow::_UpdateSettings()
{
	bool enabled = fCurrentSettings != NULL;
	fSettingsName->SetEnabled(enabled);
	fSettingsScope->SetEnabled(enabled);
	fServerAddress1->SetEnabled(enabled);
	fServerAddress2->SetEnabled(enabled);
	fEraseObjects->SetEnabled(enabled);
	fEraseRevisions->SetEnabled(enabled);
	fLogUploadTime->SetEnabled(enabled);

	if (fCurrentSettings) {
		fSettingsName->SetText(fCurrentSettings->Name().String());

		// mark the right scope item
		BString value;
		BMenu* menu = fSettingsScope->Menu();
		if (fCurrentSettings->GetValue(PROPERTY_SCOPE, value)) {
			for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
				item->SetMarked(value == item->Label());
			}
			if (!menu->FindMarked())
				menu->Superitem()->SetLabel("<select>");
		} else {
			menu->Superitem()->SetLabel("<no property>");
		}

		value = "";
		if (fCurrentSettings->GetValue(PROPERTY_SERVER_IP, value)) {
			fServerAddress1->SetText(value.String());
		} else {
			fServerAddress1->SetText("<no property>");
		}

		value = "";
		if (fCurrentSettings->GetValue(PROPERTY_SECONDARY_SERVER_IP, value)) {
			fServerAddress2->SetText(value.String());
		} else {
			fServerAddress2->SetText("<no property>");
		}

		fEraseObjects->SetValue(fCurrentSettings->Value(
			PROPERTY_ERASE_DATA_FOLDER, false));

		fEraseRevisions->SetValue(fCurrentSettings->Value(
			PROPERTY_ERASE_OLD_REVISIONS, false));

		value = "";
		if (fCurrentSettings->GetValue(PROPERTY_LOG_UPLOAD_TIME, value)) {
			fLogUploadTime->SetText(value.String());
		} else {
			fLogUploadTime->SetText("<no property>");
		}

	} else {
		fSettingsName->SetText("select an object from the list");
		fSettingsScope->Menu()->Superitem()->SetLabel("<unavailable>");
		fServerAddress1->SetText("<unavailable>");
		fServerAddress2->SetText("<unavailable>");
		fEraseObjects->SetValue(false);
		fEraseRevisions->SetValue(false);
		fLogUploadTime->SetText("<unavailable>");
	}
}




/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DisplaySettingsWindow.h"

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

#include "EditorApp.h"
#include "DisplaySettingsListView.h"
#include "CommandStack.h"
#include "CommonPropertyIDs.h"
#include "Group.h"
#include "MessageConstants.h"
#include "Playlist.h"
#include "ScrollView.h"
#include "ServerObject.h"

using std::nothrow;

enum {
	MSG_CREATE_DISPLAY_SETTINGS	= 'crcs',

	MSG_SETTINGS_SELECTED		= 'stsl',

	MSG_SET_NAME				= 'stnm',
	MSG_SET_SCOPE				= 'stsc',
	MSG_SET_WIDTH				= 'stwd',
	MSG_SET_HEIGHT				= 'stht',
};

// constructor
DisplaySettingsWindow::DisplaySettingsWindow(BRect frame, EditorApp* app)
	:
	BWindow(frame, "Display Settings", B_FLOATING_WINDOW_LOOK,
		B_FLOATING_APP_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fApp(app),
	fObjectManager(NULL),
	fCurrentSettings(NULL)
{
	_Init();
}

// destructor
DisplaySettingsWindow::~DisplaySettingsWindow()
{
	if (fObjectManager)
		fObjectManager->RemoveListener(this);

	SetCurrentSettings(NULL);
}

// #pragma mark -

// MessageReceived
void
DisplaySettingsWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREPARE_FOR_SYNCHRONIZATION:
			SetCurrentSettings(NULL);
			fListView->DeselectAll();
			message->SendReply(MSG_READY_FOR_SYNCHRONIZATION);
			break;

		case MSG_CREATE_DISPLAY_SETTINGS:
			if (fObjectManager && fObjectManager->WriteLock()) {
				ServerObject* o = new (nothrow) ServerObject("DisplaySettings");
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

		case MSG_SET_WIDTH:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_WIDTH, fSettingsWidth->Text());
				// adopt text control to the actual value been set
				BString helper;
				fCurrentSettings->GetValue(PROPERTY_WIDTH, helper);
				fSettingsWidth->SetText(helper.String());
			}
			break;
		case MSG_SET_HEIGHT:
			if (fCurrentSettings) {
				fCurrentSettings->SetValue(PROPERTY_HEIGHT, fSettingsHeight->Text());
				// adopt text control to the actual value been set
				BString helper;
				fCurrentSettings->GetValue(PROPERTY_HEIGHT, helper);
				fSettingsHeight->SetText(helper.String());
			}
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
DisplaySettingsWindow::QuitRequested()
{
	Hide();

	return false;
}

// #pragma mark -

// ObjectChanged
void
DisplaySettingsWindow::ObjectChanged(const Observable* object)
{
	if (!Lock())
		return;

	// TODO: ?

	Unlock();
}

// ObjectAdded
void
DisplaySettingsWindow::ObjectAdded(ServerObject* object, int32 index)
{
}

// ObjectRemoved
void
DisplaySettingsWindow::ObjectRemoved(ServerObject* object)
{
	if (object == fCurrentSettings)
		SetCurrentSettings(NULL);
}

// #pragma mark -

// SetObjectManager
void
DisplaySettingsWindow::SetObjectManager(ServerObjectManager* manager)
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
DisplaySettingsWindow::SetCurrentSettings(ServerObject* object)
{
	if (fCurrentSettings == object)
		return;

	if (!fObjectManager->ReadLock()) {
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
DisplaySettingsWindow::SetScopes(const BMessage* _scopes)
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
DisplaySettingsWindow::_Init()
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
DisplaySettingsWindow::_CreateGUI()
{
	fMenuBar = _CreateMenuBar();

	// clip list view
	fListView = new DisplaySettingsListView("display settings lv",
			new BMessage(MSG_SETTINGS_SELECTED), this);

	// scroll view around list view
	BScrollView* listScrollView = new BScrollView("list scroll view",
		fListView, 0, false, true, B_NO_BORDER);

	// display settings name
	fSettingsName = new BTextControl("settings name", "Settings Name", "",
		new BMessage(MSG_SET_NAME));

	// client scope(s)
	fSettingsScope = new BMenuField("scopes", "Settings Scope",
		new BPopUpMenu("all"), NULL);

	// width and height
	fSettingsWidth = new BTextControl("width", "Width", "",
		new BMessage(MSG_SET_WIDTH));
	fSettingsHeight = new BTextControl("height", "Height", "",
		new BMessage(MSG_SET_HEIGHT));

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fMenuBar)
		.AddGroup(B_HORIZONTAL)
			.Add(listScrollView)
			.Add(BGridLayoutBuilder(5.0f, 5.0f)
				.SetInsets(5.0f, 5.0f, 5.0f, 5.0f)
				.Add(fSettingsName->CreateLabelLayoutItem(), 0, 0)
				.Add(fSettingsName->CreateTextViewLayoutItem(), 1, 0)
				.Add(fSettingsScope->CreateLabelLayoutItem(), 0, 1)
				.Add(fSettingsScope->CreateMenuBarLayoutItem(), 1, 1)
				.Add(fSettingsWidth->CreateLabelLayoutItem(), 0, 2)
				.Add(fSettingsWidth->CreateTextViewLayoutItem(), 1, 2)
				.Add(fSettingsHeight->CreateLabelLayoutItem(), 0, 3)
				.Add(fSettingsHeight->CreateTextViewLayoutItem(), 1, 3)
			)
		.End()
	);

}

// _CreateMenuBar
BMenuBar*
DisplaySettingsWindow::_CreateMenuBar()
{
	BMenuBar* menuBar = new BMenuBar("menu bar");

	// Display Settings
	fClientSettingsMenu = new BMenu("Display Settings");
	BMenuItem* item = new BMenuItem("New",
		new BMessage(MSG_CREATE_DISPLAY_SETTINGS), 'N');
	fClientSettingsMenu->AddItem(item);
	fClientSettingsMenu->SetTargetForItems(this);

	menuBar->AddItem(fClientSettingsMenu);

	return menuBar;
}

// _UpdateSettings
void
DisplaySettingsWindow::_UpdateSettings()
{
	if (fCurrentSettings) {
		fSettingsName->SetText(fCurrentSettings->Name().String());
		fSettingsName->SetEnabled(true);

		fSettingsScope->SetEnabled(true);
		// mark the right scope item
		BString scope;
		if (fCurrentSettings->GetValue(PROPERTY_SCOPE, scope)) {
			BMenu* menu = fSettingsScope->Menu();
			for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
				item->SetMarked(scope == item->Label());
			}
			if (!menu->FindMarked())
				menu->Superitem()->SetLabel("<select>");
		}

		fSettingsWidth->SetEnabled(true);
		BString helper;
		fCurrentSettings->GetValue(PROPERTY_WIDTH, helper);
		fSettingsWidth->SetText(helper.String());

		fSettingsHeight->SetEnabled(true);
		helper = "";
		fCurrentSettings->GetValue(PROPERTY_HEIGHT, helper);
		fSettingsHeight->SetText(helper.String());
	} else {
		fSettingsName->SetText("select an object from the list");
		fSettingsName->SetEnabled(false);

		fSettingsScope->SetEnabled(false);
		fSettingsScope->Menu()->Superitem()->SetLabel("<unavailable>");

		fSettingsWidth->SetEnabled(false);
		fSettingsWidth->SetText("");
		fSettingsHeight->SetEnabled(false);
		fSettingsHeight->SetText("");
	}
}




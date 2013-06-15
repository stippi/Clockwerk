/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleWindow.h"

#include <string.h>
#include <stdio.h>
#include <time.h>

#include <Alert.h>
#include <File.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <ScrollView.h>
#include <String.h>

#include "common.h"
#include "support.h"
#include "support_date.h"

#include "AddObjectsCommand.h"
#include "CommandStack.h"
#include "CommonPropertyIDs.h"
#include "EditorApp.h"
#include "InfoView.h"
#include "ListLabelView.h"
#include "Playlist.h"
#include "PlaylistObjectListView.h"
#include "RemoveObjectsCommand.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "ScheduleListGroup.h"
#include "ScheduleMessages.h"
#include "ScheduleObjectListView.h"
#include "SchedulePropertiesView.h"
#include "ScheduleTopView.h"
#include "ScheduleView.h"
#include "ScrollView.h"
#include "Selection.h"
#include "ServerObjectFactory.h"
#include "StatusBar.h"
#include "TimeRangePanel.h"

enum {
	MSG_UNDO								= 'undo',
	MSG_REDO								= 'redo',

	MSG_SCHEDULE_SELECTED					= 'scds',

	MSG_CREATE_SCHEDULE						= 'crsc',
	MSG_DUPLICATE_SCHEDULE					= 'dpsc',
	MSG_REMOVE_SCHEDULE						= 'rmsc',
	MSG_RENDER_SCHEDULE_SPAN				= 'rdss',
	MSG_EXPORT_SCHEDULE_CSV					= 'xcsv',
	MSG_EXPORT_SCHEDULE_SUMMARY_BY_NAME		= 'xsmn',
	MSG_EXPORT_SCHEDULE_SUMMARY_BY_PORTION	= 'xsmp',
	MSG_FORCE_UPDATE						= 'frcu',

	MSG_PLAYLIST_SELECTED					= 'plsl',
};


struct ScheduleExportInfo {
	Schedule*	schedule;
	int32		mode;
};

// constructor
ScheduleWindow::ScheduleWindow(BRect frame)
	: BWindow(frame, "Edit Schedules", B_DOCUMENT_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS)
	, fObjectManager(NULL)
	, fObjectFactory(NULL)
	, fSOMListener(new (std::nothrow) AsyncSOMListener(this))
	, fSchedule(NULL)
	, fCommandStack(new (std::nothrow) CommandStack())
	, fSelection(new (std::nothrow) Selection())

	, fLastMouseMovedView(NULL)
	, fTimeRangePanel(NULL)
{
	frame.OffsetTo(B_ORIGIN);
	_CreateGUI(frame);

	fScheduleView->SetCommandStack(fCommandStack);
	fScheduleView->SetSelection(fSelection);
	fScheduleView->SetLocker(fObjectManager ? fObjectManager->Locker() : NULL);
	fPropertiesView->SetCommandStack(fCommandStack);
	fPropertiesView->SetSelection(fSelection);

	fPlaylistListView->SetDragCommand(MSG_DRAG_PLAYLIST);

	if (fCommandStack) {
		fCommandStack->AddObserver(this);
		ObjectChanged(fCommandStack);
	} else {
		printf("ScheduleWindow() - unable to create command stack\n");
		fatal(B_NO_MEMORY);
	}

	if (!fSelection) {
		printf("ScheduleWindow() - unable to create selection\n");
		fatal(B_NO_MEMORY);
	}

	AddShortcut('Y', 0, new BMessage(MSG_UNDO));
	AddShortcut('Y', B_SHIFT_KEY, new BMessage(MSG_REDO));
}

// destructor
ScheduleWindow::~ScheduleWindow()
{
	if (fSelection)
		fSelection->DeselectAll();

	// make sure no views are attached to any observables anymore
	fTopView->RemoveSelf();
	delete fTopView;
	fTopView = NULL;

	if (fObjectManager)
		fObjectManager->RemoveListener(fSOMListener);

	delete fSOMListener;

	if (fCommandStack)
		fCommandStack->RemoveObserver(this);

	SetSchedule(NULL);

	delete fCommandStack;
	delete fSelection;

	if (fTimeRangePanel && fTimeRangePanel->Lock())
		fTimeRangePanel->Quit();
}

// DispatchMessage
void
ScheduleWindow::DispatchMessage(BMessage* message, BHandler* target)
{
	bool handled = false;

	switch (message->what) {
		case B_MOUSE_WHEEL_CHANGED:
			handled = true;
			// fall through
		case B_MODIFIERS_CHANGED:
			// NOTE: in case multiple views register to receive
			// mouse moved messages, LastMouseMovedView() might
			// not be the view under the mouse!
			// TODO: theoretical race condition, fLastMouseMovedView
			// might not be part of the window anymore
			if (fLastMouseMovedView)
				fLastMouseMovedView->MessageReceived(message);

			break;

		case B_MOUSE_MOVED: {
			// remember the last view under the mouse,
			// see if the target of this message actually
			// contains the "view_where" mouse coords
			BView* view = dynamic_cast<BView*>(target);
			BPoint where;
			if (view && message->FindPoint("be:view_where", &where) == B_OK
				&& view->Bounds().Contains(where))
				fLastMouseMovedView = view;
			break;
		}
	}

	if (!handled)
		BWindow::DispatchMessage(message, target);
}

// MessageReceived
void
ScheduleWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PREPARE_FOR_SYNCHRONIZATION:
			SetSchedule(NULL);
			fScheduleListView->DeselectAll();
			fCommandStack->MakeEmpty();
			message->SendReply(MSG_READY_FOR_SYNCHRONIZATION);
			break;

		case MSG_SAVE:
			_MarkScheduleUnsavedIfNecessary();
			be_app->PostMessage(message);
			break;

		case MSG_FORCE_UPDATE:
			_ForceUpdate();
			break;

		case MSG_CREATE_SCHEDULE:
			_CreateSchedule();
			break;
		case MSG_DUPLICATE_SCHEDULE:
			_DuplicateSchedule();
			break;
		case MSG_REMOVE_SCHEDULE:
			_RemoveSchedule();
			break;

		case MSG_SCHEDULE_SELECTED: {
			ServerObject* object;
			if (message->FindPointer("object", (void**)&object) == B_OK) {
				SetSchedule(dynamic_cast<Schedule*>(object));
			} else {
				SetSchedule(NULL);
			}
			break;
		}

		case MSG_PLAYLIST_SELECTED: {
			ServerObject* object;
			if (message->FindPointer("object", (void**)&object) == B_OK) {
				Playlist* playlist = dynamic_cast<Playlist*>(object);
				fScheduleView->HighlightPlaylist(playlist);
				bool invoked;
				if (message->FindBool("invoked", &invoked) == B_OK && invoked)
					fScheduleView->SelectAllItems(playlist);
			}
			break;
		}

		case MSG_UNDO:
			if (fObjectManager->WriteLock()) {
				fCommandStack->Undo();
				fObjectManager->WriteUnlock();
			}
			break;
		case MSG_REDO:
			if (fObjectManager->WriteLock()) {
				fCommandStack->Redo();
				fObjectManager->WriteUnlock();
			}
			break;

		case AsyncSOMListener::MSG_OBJECT_ADDED: {
			ServerObject* object;
			int32 index;
			if (message->FindPointer("object", (void**)&object) == B_OK
				&& message->FindInt32("index", &index) == B_OK) {
				_ObjectAdded(object, index);
			}
			break;
		}
		case AsyncSOMListener::MSG_OBJECT_REMOVED: {
			ServerObject* object;
			if (message->FindPointer("object", (void**)&object) == B_OK) {
				_ObjectRemoved(object);
			}
			break;
		}

		case MSG_RENDER_SCHEDULE_SPAN:
			_RenderSchedule(message);
			break;

		case MSG_EXPORT_SCHEDULE_CSV:
		case MSG_EXPORT_SCHEDULE_SUMMARY_BY_NAME:
		case MSG_EXPORT_SCHEDULE_SUMMARY_BY_PORTION: {
			if (!fSchedule)
				break;
			ScheduleExportInfo* info = new ScheduleExportInfo;
			thread_id thread = spawn_thread(_ExportScheduleEntry,
				"schedule exporter", B_LOW_PRIORITY, info);
			if (thread < 0) {
				delete info;
				break;
			}
			info->schedule = new (std::nothrow) Schedule(*fSchedule, true);
			info->mode = message->what;
			resume_thread(thread);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// QuitRequested
bool
ScheduleWindow::QuitRequested()
{
	Hide();

	return false;
}

// Hide
void
ScheduleWindow::Hide()
{
	if (fTimeRangePanel && fTimeRangePanel->Lock()) {
		fTimeRangePanel->Quit();
		fTimeRangePanel = NULL;
	}

	BWindow::Hide();
}

// #pragma mark -

// ObjectChanged
void
ScheduleWindow::ObjectChanged(const Observable* object)
{
	if (object == fCommandStack) {
		// relable Undo item and update enabled status
		BString label("Undo");
		fUndoMI->SetEnabled(fCommandStack->GetUndoName(label));
		if (fUndoMI->IsEnabled())
			fUndoMI->SetLabel(label.String());
		else
			fUndoMI->SetLabel("<nothing to undo>");

		// relable Redo item and update enabled status
		label.SetTo("Redo");
		fRedoMI->SetEnabled(fCommandStack->GetRedoName(label));
		if (fRedoMI->IsEnabled())
			fRedoMI->SetLabel(label.String());
		else
			fRedoMI->SetLabel("<nothing to redo>");
	}
}

// #pragma mark -

// PrepareForQuit
void
ScheduleWindow::PrepareForQuit()
{
	if (Lock()) {
		_MarkScheduleUnsavedIfNecessary();
		Unlock();
	}
}

// SetObjectManager
void
ScheduleWindow::SetObjectManager(ServerObjectManager* manager)
{
	if (fObjectManager == manager)
		return;

	if (fObjectManager)
		fObjectManager->RemoveListener(fSOMListener);

	fObjectManager = manager;

	// hook up the list views to clip library
	fScheduleListView->SetObjectLibrary(fObjectManager);
	fPlaylistListView->SetObjectLibrary(fObjectManager);
	fScheduleView->SetLocker(fObjectManager ? fObjectManager->Locker() : NULL);

	if (fObjectManager)
		fObjectManager->AddListener(fSOMListener);
}

// SetObjectFactory
void
ScheduleWindow::SetObjectFactory(ServerObjectFactory* factory)
{
	fObjectFactory = factory;
}

// SetSchedule
void
ScheduleWindow::SetSchedule(Schedule* schedule)
{
	if (fSchedule == schedule)
		return;

	if (!fObjectManager->ReadLock()) {
		printf("ScheduleWindow::SetSchedule() - "
			"unable to readlock object library\n");
		return;
	}

	if (fSelection)
		fSelection->DeselectAll();

	if (fSchedule) {
		fSchedule->RemoveObserver(this);
		_MarkScheduleUnsavedIfNecessary();
		fSchedule->Release();
	}

	fSchedule = schedule;

	fCommandStack->Save();

	if (fSchedule) {
		fSchedule->Acquire();
		fSchedule->AddObserver(this);
		fSchedule->SanitizeStartFrames();
	}

	if (fTopView) {
		fScheduleView->SetSchedule(fSchedule);
		fPropertiesView->SetSchedule(fSchedule);

		const char* defaultStatus;
		if (fSchedule)
			defaultStatus = "Drag playlists into the schedule. You can hold <Ctrl> to insert them at any time (possibly cutting existing items in half).";
		else
			defaultStatus = "Select or create a schedule.";

		fStatusBar->SetDefaultMessage(defaultStatus);

		fDuplicateMI->SetEnabled(fSchedule != NULL);
		fRemoveMI->SetEnabled(fSchedule != NULL);
		fRenderMI->SetEnabled(fSchedule != NULL);
		fExportCSVMI->SetEnabled(fSchedule != NULL);
		fExportSummaryNameMI->SetEnabled(fSchedule != NULL);
		fExportSummaryPortionMI->SetEnabled(fSchedule != NULL);
		fForceUpdateMI->SetEnabled(fSchedule != NULL);
	}

	fObjectManager->ReadUnlock();
}

// SetScopes
void
ScheduleWindow::SetScopes(const BMessage* scopes)
{
	fPropertiesView->SetScopes(scopes);
	fScheduleListGroup->SetScopes(scopes);
}

// #pragma mark -

// StoreSettings
void
ScheduleWindow::StoreSettings(BMessage* archive) const
{
	fPlaylistListView->StoreSettings(archive, "playlist lv settings");
}

// RestoreSettings
void
ScheduleWindow::RestoreSettings(BMessage* archive)
{
	fPlaylistListView->RestoreSettings(archive, "playlist lv settings");
}

// #pragma mark -

// _CreateGUI
void
ScheduleWindow::_CreateGUI(BRect frame)
{
	// create the topview which layouts all the controls
	fTopView = new ScheduleTopView(frame);

	BRect menuFrame(frame);
	menuFrame.bottom = frame.top + 15;
	BMenuBar* menuBar = _CreateMenuBar(menuFrame);
	fTopView->AddMenuBar(menuBar);
//	float menuWidth, menuHeight;
//	menuBar->GetPreferredSize(&menuWidth, &menuHeight);
//	menuBar->ResizeTo(menuFrame.Width(), menuHeight);

	fStatusBar = new StatusBar("Select or create a schedule.", true);
	fTopView->AddStatusBar(fStatusBar);

	// schedule list view
	fScheduleListView = new ScheduleObjectListView("schedule lv",
			new BMessage(MSG_SCHEDULE_SELECTED), this);
	fScheduleListGroup = new ScheduleListGroup(fScheduleListView);

	fTopView->AddScheduleListGroup(fScheduleListGroup);

	// playlist list label view
	ListLabelView* labelView = new ListLabelView(BRect(0, 0, 199, 15),
		"playlist label view", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP,
		"Available Playlists");

	labelView->ResizeToPreferred();
	float labelHeight = labelView->Bounds().Height();

	// playlist list view
	fPlaylistListView = new PlaylistObjectListView("playlist lv",
			new BMessage(MSG_PLAYLIST_SELECTED), this);

	// scroll view around list view
	BRect playlistFrame(0, 0, 199, 99);
	BView* playlistGroup = new BView(playlistFrame, "playlist group",
		B_FOLLOW_NONE, 0);
	playlistGroup->AddChild(labelView);
	playlistFrame.top += labelHeight + 1;
	playlistGroup->AddChild(new ScrollView(fPlaylistListView,
		SCROLL_VERTICAL | SCROLL_HORIZONTAL_MAGIC
			| SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS,
		playlistFrame, "playlist scrollview", B_FOLLOW_ALL, 0,
		B_PLAIN_BORDER, BORDER_RIGHT));

//	InfoView* infoView = new InfoView(
//		BRect(0, 99 - B_H_SCROLL_BAR_HEIGHT + 1, 199, 99), "info view");
//	playlistGroup->AddChild(infoView);

	fTopView->AddPlaylistListGroup(playlistGroup);

	fPropertiesView = new SchedulePropertiesView();
	fTopView->AddPropertyGroup(fPropertiesView);

	fScheduleView = new ScheduleView(BRect(0, 0, 99, 99), "schedule view");


	frame.bottom += B_H_SCROLL_BAR_HEIGHT;
	frame.right += B_V_SCROLL_BAR_WIDTH;
	ScrollView* scrollView = new ScrollView(fScheduleView,
		SCROLL_HORIZONTAL | SCROLL_VERTICAL
			| SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS,
		frame, "schedule scroll view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER);
	fTopView->AddScheduleGroup(scrollView);
	fScheduleView->MakeFocus(true);

	AddChild(fTopView);

	// take some additional care of the scroll bars that scroll
	// the list views
	if (BScrollBar* scrollBar = fScheduleListView->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 2);
	}
	if (BScrollBar* scrollBar = fPlaylistListView->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 2);
	}
}

// _CreateMenuBar
BMenuBar*
ScheduleWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "main menu");
	BMenu* fileMenu = new BMenu("Schedule");

	BMenuItem* item = new BMenuItem("New",
		new BMessage(MSG_CREATE_SCHEDULE), 'N');
	fileMenu->AddItem(item);

	item = new BMenuItem("Save", new BMessage(MSG_SAVE), 'S');
	fileMenu->AddItem(item);

	fileMenu->AddSeparatorItem();

	fDuplicateMI = new BMenuItem("Duplicate",
		new BMessage(MSG_DUPLICATE_SCHEDULE), 'D');
	fDuplicateMI->SetEnabled(false);
	fileMenu->AddItem(fDuplicateMI);

	fRemoveMI = new BMenuItem("Remove", new BMessage(MSG_REMOVE_SCHEDULE));
	fRemoveMI->SetEnabled(false);
	fileMenu->AddItem(fRemoveMI);

	fileMenu->AddSeparatorItem();

	fRenderMI = new BMenuItem("Render"B_UTF8_ELLIPSIS,
		new BMessage(MSG_RENDER_SCHEDULE_SPAN), 'R');
	fRenderMI->SetEnabled(false);
	fileMenu->AddItem(fRenderMI);

	fExportCSVMI = new BMenuItem("Export as CSV",
		new BMessage(MSG_EXPORT_SCHEDULE_CSV));
	fExportCSVMI->SetEnabled(false);
	fileMenu->AddItem(fExportCSVMI);

	fExportSummaryNameMI = new BMenuItem("Export Summary (by Name)",
		new BMessage(MSG_EXPORT_SCHEDULE_SUMMARY_BY_NAME));
	fExportSummaryNameMI->SetEnabled(false);
	fileMenu->AddItem(fExportSummaryNameMI);

	fExportSummaryPortionMI = new BMenuItem("Export Summary (by Portion)",
		new BMessage(MSG_EXPORT_SCHEDULE_SUMMARY_BY_PORTION));
	fExportSummaryPortionMI->SetEnabled(false);
	fileMenu->AddItem(fExportSummaryPortionMI);

	fileMenu->AddSeparatorItem();

	fForceUpdateMI = new BMenuItem("Force Update",
		new BMessage(MSG_FORCE_UPDATE), 'U');
	fForceUpdateMI->SetEnabled(false);
	fileMenu->AddItem(fForceUpdateMI);

	item = new BMenuItem("Close", new BMessage(B_QUIT_REQUESTED), 'W');
	fileMenu->AddItem(item);

	menuBar->AddItem(fileMenu);

	BMenu* editMenu = new BMenu("Edit");
	BMessage* message = new BMessage(MSG_UNDO);
	fUndoMI = new BMenuItem("Undo", message, 'Z');
	editMenu->AddItem(fUndoMI);
	message = new BMessage(MSG_REDO);
	fRedoMI = new BMenuItem("Redo", message, 'Z', B_SHIFT_KEY);
	editMenu->AddItem(fRedoMI);

	menuBar->AddItem(editMenu);

	fileMenu->SetTargetForItems(this);
	editMenu->SetTargetForItems(this);

	return menuBar;
}

//// _UpdateSchedule
//void
//ClientSettingsWindow::_UpdateSchedule()
//{
//	if (fCurrentSettings) {
//		fSettingsName->SetText(fCurrentSettings->Name().String());
//		fSettingsName->SetEnabled(true);
//
//		fSettingsScope->SetEnabled(true);
//		// mark the right scope item
//		BString scope;
//		if (fCurrentSettings->GetValue(PROPERTY_SCOPE, scope)) {
//			BMenu* menu = fSettingsScope->Menu();
//			for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
//				item->SetMarked(scope == item->Label());
//			}
//			if (!menu->FindMarked())
//				menu->Superitem()->SetLabel("<select>");
//		}
//
//		fPlaylistsField->SetEnabled(true);
//		// mark the right playlist item
//		BString playlistRef;
//		if (fCurrentSettings->GetValue(PROPERTY_REFERENCED_PLAYLIST,
//									   playlistRef)) {
//			BMenu* menu = fPlaylistsField->Menu();
//			for (int32 i = 0; BMenuItem* item = menu->ItemAt(i); i++) {
//				BMessage* m = item->Message();
//				BString soid;
//				item->SetMarked(m->FindString("soid", &soid) >= B_OK
//								&& soid == playlistRef);
//			}
//			if (!menu->FindMarked())
//				menu->Superitem()->SetLabel("<select>");
//		}
//	} else {
//		fSettingsName->SetText("select an object from the list");
//		fSettingsName->SetEnabled(false);
//
//		fSettingsScope->SetEnabled(false);
//		fSettingsScope->Menu()->Superitem()->SetLabel("<unavailable>");
//
//		fPlaylistsField->SetEnabled(false);
//		fPlaylistsField->Menu()->Superitem()->SetLabel("<unavailable>");
//	}
//}

// _CreateSchedule
void
ScheduleWindow::_CreateSchedule()
{
	if (fObjectManager) {
		ServerObject* objects[1];
		objects[0] = new (std::nothrow) Schedule();
		objects[0]->SetName("<new Schedule>");
		fCommandStack->Perform(
			new (std::nothrow) AddObjectsCommand(fObjectManager, objects,
				1, NULL));
	}
}

// _DuplicateSchedule
void
ScheduleWindow::_DuplicateSchedule()
{
	if (fObjectManager && fObjectFactory && fSchedule) {
		ServerObject* objects[1];
		objects[0] = fObjectFactory->InstantiateClone(fSchedule,
			fObjectManager);
		if (objects[0]) {
			// overwrite status property with "draft"
			BString value;
			value << PLAYLIST_STATUS_DRAFT;
			objects[0]->SetValue(PROPERTY_STATUS, value.String());
		}
		fCommandStack->Perform(new (std::nothrow) AddObjectsCommand(
			fObjectManager, objects, 1, NULL));
	}
}

// _RemoveSchedule
void
ScheduleWindow::_RemoveSchedule()
{
	if (fObjectManager && fSchedule) {
		ServerObject* objects[1];
		objects[0] = fSchedule;
		fCommandStack->Perform(new (std::nothrow) RemoveObjectsCommand(
			fObjectManager, objects, 1, NULL));
	}
}

// _ObjectAdded
void
ScheduleWindow::_ObjectAdded(ServerObject* object, int32 index)
{
	// in window thread
//	if (fSchedule == NULL) {
//		Schedule* schedule = dynamic_cast<Schedule*>(object);
//		if (schedule)
//			SetSchedule(schedule);
//	}
}

// _ObjectRemoved
void
ScheduleWindow::_ObjectRemoved(ServerObject* object)
{
	// in window thread
	if (object == fSchedule)
		SetSchedule(NULL);
}

// _RenderSchedule
void
ScheduleWindow::_RenderSchedule(const BMessage* message)
{
	if (!fSchedule)
		return;

	const char* startTime;
	const char* endTime;
	if (message->FindString("start time", &startTime) == B_OK
		&& message->FindString("end time", &endTime) == B_OK) {
		// convert time code strings
		int64 startFrame = day_time_to_frame(startTime);
		int64 endFrame = day_time_to_frame(endTime);
		// sanitize
		if (startFrame > endFrame) {
			int64 temp = startFrame;
			startFrame = endFrame;
			endFrame = temp;
		}
		// generate playlist
		Playlist* playlist
			= fSchedule->GeneratePlaylist(startFrame, endFrame);
		// make app render the playlist
		if (playlist) {
playlist->PrintToStream();
			BMessage renderMessage(MSG_RENDER_PLAYLIST);
			renderMessage.AddPointer("playlist", playlist);
			if (be_app_messenger.SendMessage(&renderMessage) < B_OK) {

				playlist->Release();
			}
		} else {
			char errorMessage[1024];
			sprintf(errorMessage, "Failed to generate playlist from schedule.\n"
				"Time codes: '%s' -> '%s'\nFrames: %lld -> %lld\n",
				startTime, endTime, startFrame, endFrame);

			BAlert* alert = new BAlert("error", errorMessage, NULL, NULL, "Ok");
			alert->Go(NULL);
		}
	} else if (message->HasBool("cancel")) {
		if (fTimeRangePanel && fTimeRangePanel->Lock()) {
			fTimeRangePanel->Quit();
			fTimeRangePanel = NULL;
		}
	} else {
		if (!fTimeRangePanel) {
			fTimeRangePanel = new TimeRangePanel(Frame(),
				0, 0, this, new BMessage(MSG_RENDER_SCHEDULE_SPAN));
			fTimeRangePanel->Show();
		} else {
			if (fTimeRangePanel->Lock()) {
				fTimeRangePanel->Activate();
				fTimeRangePanel->Unlock();
			}
		}
	}
}

// _MarkScheduleUnsavedIfNecessary
void
ScheduleWindow::_MarkScheduleUnsavedIfNecessary()
{
	if (fSchedule && !fCommandStack->IsSaved()) {
		fSchedule->SetDataSaved(false);
		fCommandStack->Save();
	}
}

// _ForceUpdate
void
ScheduleWindow::_ForceUpdate()
{
	if (!fSchedule)
		return;

	AutoWriteLocker locker(fObjectManager->Locker());

	fObjectManager->ResolveDependencies();

	// search for "Playlist" objects
	int32 count = fObjectManager->CountObjects();
	for (int32 i = 0; i < count; i++) {
		Playlist* playlist = dynamic_cast<Playlist*>(
			fObjectManager->ObjectAtFast(i));
		if (playlist)
			playlist->ValidateItemLayout();
	}

	fSchedule->SanitizeStartFrames();
}

// #pragma mark -

// _ExportScheduleEntry
/*static*/ int32
ScheduleWindow::_ExportScheduleEntry(void* cookie)
{
	ScheduleExportInfo* info = (ScheduleExportInfo*)cookie;
	status_t ret;
	switch (info->mode) {
		default:
		case MSG_EXPORT_SCHEDULE_CSV:
			ret = _ExportScheduleCSV(info->schedule);
			break;

		case MSG_EXPORT_SCHEDULE_SUMMARY_BY_NAME:
			ret = _ExportScheduleSummary(info->schedule, SORT_BY_NAME);
			break;
		case MSG_EXPORT_SCHEDULE_SUMMARY_BY_PORTION:
			ret = _ExportScheduleSummary(info->schedule, SORT_BY_PORTION);
			break;
	}
	if (ret < B_OK) {
		BString message("Failed to export schedule as CSV data.\n\nError: ");
		message << strerror(ret) << "\n";
		BAlert* alert = new BAlert("error", message.String(), "Bummer", NULL,
			NULL, B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
		alert->Go();
	}
	delete info->schedule;
	delete info;
	return 0;
}

static void
make_schedule_filename(char* name, const Schedule* schedule, const char* path,
	const char* extension)
{
	time_t t = time(NULL);
	tm time = *localtime(&t);
	sprintf(name, "%sSchedule_%s_v%ld_%0*d%0*d%0*d.%s", path,
		schedule->Name().String(), schedule->Version(),
		4, time.tm_year + 1900, 2, time.tm_mon + 1, 2, time.tm_mday,
		extension);
}

// _ExportScheduleCSV
/*static*/ status_t
ScheduleWindow::_ExportScheduleCSV(const Schedule* schedule)
{
	if (!schedule)
		return B_NO_MEMORY;

	char fileName[B_PATH_NAME_LENGTH];
	make_schedule_filename(fileName, schedule, "/boot/home/Desktop/", "csv");

	BFile file(fileName, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	status_t ret = file.InitCheck();
	if (ret < B_OK) {
		printf("ScheduleWindow::_ExportScheduleCSV() - "
			"failed to create output file: %s\n", strerror(ret));
		return ret;
	}

	int32 count = schedule->CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* item = schedule->ItemAtFast(i);

		uint64 startFrame = item->StartFrame();
		uint32 hours = startFrame / (60 * 60 * 25);
		startFrame -= hours * 60 * 60 * 25;
		uint32 minutes = startFrame / (60 * 25);
		startFrame -= minutes * 60 * 25;
		uint32 seconds = startFrame / 25;
		startFrame -= seconds * 25;

		BString line;
		line << hours << ',';
		line << minutes << ',';
		line << seconds << ',';
		line << startFrame << ',';
		Playlist* playlist = item->Playlist();
		if (playlist)
			line << playlist->Name();
		else
			line << "<no playlist>";
		line << '\n';

		ssize_t written = file.Write(line.String(), line.Length());
		if (written != line.Length()) {
			if (written < 0)
				return (status_t)written;
			return B_IO_ERROR;
		}
	}

	return B_OK;
}

// ----------------------------
// ExportText utility functions
// ----------------------------

// append_string_left
static inline
void
append_string_left(BString& string, const BString& toAppend, int32 length)
{
	int32 appendLength = std::min(toAppend.Length(), length);
	string.Append(toAppend, appendLength);
	if (length > appendLength)
		string.Append(' ', length - appendLength);
}

// append_string_right
static inline
void
append_string_right(BString& string, const BString& toAppend, int32 length)
{
	int32 appendLength = std::min(toAppend.Length(), length);
	if (length > appendLength)
		string.Append(' ', length - appendLength);
	string.Append(toAppend, appendLength);
}

// append_string_center
static inline
void
append_string_center(BString& string, const BString& toAppend, int32 length)
{
	int32 appendLength = std::min(toAppend.Length(), length);
	int32 left = length - appendLength;
	int32 spacing = left / 2;
	if (spacing > 0)
		string.Append(' ', spacing);
	string.Append(toAppend, appendLength);
	spacing = left - spacing;
	if (spacing > 0)
		string.Append(' ', spacing);
}

// write_to_io
static inline
status_t
write_to_io(BDataIO& io, const BString& string)
{
	int32 length = string.Length();
	if (length > 0) {
		ssize_t ret = io.Write((void*)string.String(), length);
		if (ret != length) {
			if (ret >= B_OK)
				ret = B_ERROR;
			return (status_t)ret;
		}
	}
	return B_OK;
}


struct PlaylistSummaryEntry {
	BString name;
	uint64 duration;
	uint64 totalDuration;
	uint32 occurances;
};


static int
compare_items_name(const void* _a, const void* _b)
{
	PlaylistSummaryEntry* a = *(PlaylistSummaryEntry**)_a;
	PlaylistSummaryEntry* b = *(PlaylistSummaryEntry**)_b;
	return strcmp(a->name.String(), b->name.String());
}

static int
compare_items_portion(const void* _a, const void* _b)
{
	PlaylistSummaryEntry* a = *(PlaylistSummaryEntry**)_a;
	PlaylistSummaryEntry* b = *(PlaylistSummaryEntry**)_b;
	if (a->totalDuration > b->totalDuration)
		return -1;
	else if (a->totalDuration < b->totalDuration)
		return 1;
	return 0;
}


// _ExportScheduleSummary
/*static*/ status_t
ScheduleWindow::_ExportScheduleSummary(const Schedule* schedule,
	SortMode sortMode)
{
	// NOTE: Does not check for overflow of durations.

	if (!schedule)
		return B_NO_MEMORY;

	typedef HashMap<HashKey32<Playlist*>, PlaylistSummaryEntry*> SummaryMap;
	SummaryMap map;

	uint64 totalDuration = 0;
	status_t ret = B_OK;

	int32 itemCount = schedule->CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		ScheduleItem* item = schedule->ItemAtFast(i);
		Playlist* playlist = item->Playlist();
		if (!playlist && (i == 0 || i == itemCount - 1)) {
			// a pause in the middle of the schedule
			// NOTE:theoretically, there could be multiple adjacent
			// pause items in the beginning or at the end... but
			// we ignore this useless case here
			continue;
		}

		PlaylistSummaryEntry* entry = NULL;
		if (!map.ContainsKey(playlist)) {
			entry = new (std::nothrow) PlaylistSummaryEntry;
			if (!entry || map.Put(playlist, entry) < B_OK) {
				delete entry;
				ret = B_NO_MEMORY;
				break;
			}

			entry->duration = item->Duration();
			entry->totalDuration = 0;
			entry->occurances = 0;
			entry->name = playlist ? playlist->Name() : BString("<pause>");
		} else
			entry = map.Get(playlist);

		totalDuration += item->Duration();
		entry->totalDuration += item->Duration();
		entry->occurances += 1;
	}

	char fileName[B_PATH_NAME_LENGTH];
	make_schedule_filename(fileName, schedule, "/boot/home/Desktop/", "txt");

	BFile file(fileName, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	if (ret >= B_OK) {
		ret = file.InitCheck();
		if (ret < B_OK) {
			printf("ScheduleWindow::_ExportScheduleSummary() - "
				"failed to create output file: %s\n", strerror(ret));
		}
	}

	// column labels and widths
	const char* nameColumnLabel = "Playlistname";
	const char* occuranceColumnLabel = "Anzahl";
	const char* timeSingleColumnLabel = "Zeit einzeln";
	const char* timeTotalColumnLabel = "Zeit gesamt";
	const char* portionColumnLabel = "Anteil";

	const int32 nameColumnWidth = 40;
	const int32 occuranceColumnWidth = 8;
	const int32 timeColumnWidth = 16;
	const int32 portionColumnWidth = 11;

	// write header
	BString output;
	append_string_left(output, BString(nameColumnLabel),
		nameColumnWidth);
	append_string_right(output, BString(occuranceColumnLabel),
		occuranceColumnWidth);
	append_string_right(output, BString(timeSingleColumnLabel),
		timeColumnWidth);
	append_string_right(output, BString(timeTotalColumnLabel),
		timeColumnWidth);
	append_string_right(output, BString(portionColumnLabel),
		portionColumnWidth);
	output << '\n';
	if (ret >= B_OK)
		ret = write_to_io(file, output);

	BList list;
	SummaryMap::Iterator iterator = map.GetIterator();
	while (iterator.HasNext())
		list.AddItem(iterator.Next().value);

	switch (sortMode) {
		default:
		case SORT_BY_NAME:
			list.SortItems(compare_items_name);
			break;
		case SORT_BY_PORTION:
			list.SortItems(compare_items_portion);
			break;
	}

	itemCount = list.CountItems();
	for (int32 i = 0; i < itemCount; i++) {
		PlaylistSummaryEntry* entry = (PlaylistSummaryEntry*)list.ItemAtFast(i);
		if (ret >= B_OK) {
			output = "";
			append_string_left(output, entry->name, nameColumnWidth);

			BString occurances;
			occurances << entry->occurances;
			append_string_right(output, occurances, occuranceColumnWidth);

			BString time = string_for_frame(entry->duration);
			append_string_right(output, time, timeColumnWidth);

			time = string_for_frame(entry->totalDuration);
			append_string_right(output, time, timeColumnWidth);

			char portion[32];
			sprintf(portion, "%.2f%%",
				100.0 * entry->totalDuration / totalDuration);
			append_string_right(output, BString(portion), portionColumnWidth);

			output << '\n';
			ret = write_to_io(file, output);
		}
		delete entry;
	}

	if (ret >= B_OK) {
		output = "\n";
		output << "Gesamtdauer: ";
		BString duration = string_for_frame(totalDuration);
		output << duration << '\n';
		output << "Verschiedene Playlisten: " << itemCount << '\n';
		ret = write_to_io(file, output);
	}

	return ret;
}

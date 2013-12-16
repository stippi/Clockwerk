/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "EditorApp.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Alert.h>
#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FilePanel.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>

#include "common.h"
#include "support_settings.h"
#include "support_ui.h"

#include "AddObjectsCommand.h"
#include "AttributeServerObjectManager.h"
#include "AutoDeleter.h"
#include "BBP.h"
#include "Debug.h"
#include "Document.h"
#include "ClipObjectFactory.h"
#include "CommonPropertyIDs.h"
#include "FileBasedClip.h"
#include "InitProgressPanel.h"
#include "MainWindow.h"
#include "OptionProperty.h"
#include "Playlist.h"
#include "Property.h"
#include "RenderJob.h"
#include "RenderPreset.h"
#include "RenderSettingsWindow.h"
#include "StatusOutput.h"

#include "MessageConstants.h"

using std::nothrow;

static const char* kCurrentDocKey = "current doc id";
static const char* kScopesKey = "scopes";
static const char* kWindowFrameKey = "main window frame";

enum {
	MSG_LIST_SCOPES					= 'lssp',

	MSG_RENDER_SETTINGS_ACQUIRED	= 'rsac',
	MSG_RENDER_FILE_ACQUIRED		= 'rsfl',
};


// constructor
EditorApp::EditorApp()
	: ClockwerkApp(kClockwerkMimeSig)
	, fEditorSettings()
	, fMainWindow(NULL)
	, fRenderSettingsWindow(NULL)
	, fErrorLogWindow(NULL)

	, fOpenPanel(NULL)
	, fSavePanel(NULL)
	, fLastPanel(NULL)
{
}

// destructor
EditorApp::~EditorApp()
{
	delete fOpenPanel;
	delete fSavePanel;

	delete fObjectFactory;
}

// #pragma mark -

// QuitRequested
bool
EditorApp::QuitRequested()
{
	// the schedule window maintains it's own command
	// stack, and might have unsaved changes...
	if (_SynchronizeClipsOnDisk(false, true, true) == USER_CANCELED)
		return false;

	_SaveSettings();

	if (fMainWindow != NULL) {
		fMainWindow->Lock();
		fMainWindow->PrepareForQuit();
		fMainWindow->Quit();
		fMainWindow = NULL;
	}

	if (fRenderSettingsWindow != NULL) {
		fRenderSettingsWindow->Lock();
		fRenderSettingsWindow->SaveSettings();
		fRenderSettingsWindow->Quit();
		fRenderSettingsWindow = NULL;
	}

	return true;
}

// MessageReceived
void
EditorApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case BBP_SEND_BBITMAP: {
			// convert to normal B_REFS_RECEIVED format (Sander Stocks
			// unfortunately called it "ref" without the 's')
			entry_ref ref;
			for (int32 i = 0; message->FindRef("ref", i, &ref) == B_OK; i++)
				message->AddRef("refs", &ref);
			message->RemoveName("ref");
			// fall through
		}
		case B_SIMPLE_DATA:
			RefsReceived(message);
			break;

		case MSG_NEW: {
			// lock the mainwindow before proceeding, see MSG_OPEN
			if (!fMainWindow->Lock())
				break;

			status_t ret = _NewDocument();
			if (ret < B_OK)
				print_error("error creating new document: %s\n",
							strerror(ret));

			fMainWindow->Unlock();
			break;
		}

		case MSG_OPEN: {
			// NOTE: the serverID is what is actually used
			// right now, the playlist is already loaded in
			// the ServerObjectManager, we simply need to
			// switch to the new playlist

			entry_ref ref;
			BString newPlaylistID;
			if (message->FindString("soid", &newPlaylistID) == B_OK) {
				// this is the normal way of retrieving the
				// entry_ref
				if (fObjectLibrary->GetRef(newPlaylistID, ref) < B_OK) {
					printf("MSG_OPEN - fObjectLibrary->GetRef() failed\n");
					break;
				}
			} else {
				// fall back to searching for the entry_ref in
				// the message
				if (message->FindRef("refs", &ref) < B_OK)
					break;
			}

			status_t ret;
			if (newPlaylistID.Length() > 0) {
				if (fDocumentID != newPlaylistID) {
					// if the playlist is different from the currently
					// loaded playlist, ask the user if he wants to
					// save all the changes before
					if (_SynchronizeClipsOnDisk(true) == USER_CANCELED)
						break;
					ret = _OpenPlaylist(newPlaylistID);
				} else {
					// nothing to do
					printf("MSG_OPEN - same playlist\n");
					break;
				}
			} else {
				// NOTE: currently not used, playlists are always
				// "loaded" from the existing clip library, not from
				// refs
//				BAutolock locker(fMainWindow);
//				ret = _OpenDocument(fDocument, &ref);
//				if (ret == B_OK)
//					fDocumentID = fDocument->Playlist()->ID();
ret = B_ERROR;
			}
			if (ret < B_OK) {
				print_error("MSG_OPEN - error opening document: %s\n", strerror(ret));
			}

			break;
		}

		case MSG_SAVE: {
			_SynchronizeClipsOnDisk(false, false);

//			// NOTE: this is currently not used anymore
//			entry_ref ref;
//			if (_GetRefFromFilePanelMessage(message, &ref) == B_OK) {
//				status_t status = _SaveDocument(fDocument, &ref);
//				if (status < B_OK) {
//					print_error("MSG_SAVE - _SaveDocument(): %s\n",
//								strerror(status));
//				}
//			} else {
//				if (fDocument->Ref()) {
//					status_t status = _SaveDocument(fDocument,
//													fDocument->Ref());
//					if (status < B_OK) {
//						print_error("MSG_SAVE - _SaveDocument(): %s\n",
//									strerror(status));
//					}
//				} else {
//					_ShowSavePanel();
//				}
//			}
			break;
		}

		case MSG_RENDER_PLAYLIST:
			// show render settings window
			if (fRenderSettingsWindow->Lock()) {
				BMessage* renderPanelMessage = new BMessage(*message);
				renderPanelMessage->what = MSG_RENDER_SETTINGS_ACQUIRED;
				fRenderSettingsWindow->SetMessage(renderPanelMessage);
				fRenderSettingsWindow->Show();
				fRenderSettingsWindow->Unlock();
			}
			break;

		case MSG_RENDER_SETTINGS_ACQUIRED: {
			// show "render" file panel
			BMessage renderPanelMessage(*message);
			renderPanelMessage.what = MSG_RENDER_FILE_ACQUIRED;
			_ShowSavePanel(&renderPanelMessage,
						   "Render Playlist", "Render");
			break;
		}
		
		case MSG_RENDER_FILE_ACQUIRED: {
			// start render job
			entry_ref ref;
			if (_GetRefFromFilePanelMessage(message, &ref) < B_OK)
				break;

			const RenderPreset* preset
				= fRenderSettingsWindow->RenderPresetInfo();
			// either extract the playlist from the message (ownership
			// is transfered to us in this case) or clone the document
			// playlist, in which case the ownership is with us too
			// the RenderJob will take care of releasing this playlist
			Playlist* playlist;
			if (message->FindPointer("playlist", (void**)&playlist) < B_OK)
				playlist = new Playlist(*fDocument->Playlist(), true);

			BString docName = playlist->Name();
			int32 startFrame = 0;
			int32 endFrame = playlist->Duration() - 1;
			float fps = 25.0;
			RenderJob* job = new RenderJob(fMainWindow,
										   docName.String(),
										   startFrame,
										   endFrame,
										   ref,
										   preset,
										   fps,
										   playlist);
			job->Go();
			break;
		}

		case MSG_LIST_SCOPES: {
			BMessage listing;
			if (message->FindMessage("listing", &listing) == B_OK)
				_HandleScopes(&listing);
			break;
		}

		case MSG_CREATE_OBJECT: {
			BString id; // dummy id
			BString type;
			if (message->FindString("type", &type) < B_OK)
				break;
// TODO: async notifications in main window code!
if (fMainWindow->Lock()) {
			ServerObject* object = fObjectFactory->Instantiate(
				type, id, fObjectLibrary);
			if (object) {
				ServerObject* objects[1];
				objects[0] = object;
				fDocument->CommandStack()->Perform(
					new (nothrow) AddObjectsCommand(fObjectLibrary,
						objects, 1, fDocument->ClipSelection()));
			}
fMainWindow->Unlock();
}
			break;
		}

		case MSG_RESOLVE_DEPENDENCIES:
			_ResolveDependencies();
			break;

		default:
			ClockwerkApp::MessageReceived(message);
			break;
	}
}

// ReadyToRun
void
EditorApp::ReadyToRun()
{
	_LoadSettings();

	status_t ret = fEditorSettings.Init();
	if (ret < B_OK) {
		print_error("failed to load xml settings: %s\n", strerror(ret));
	}

	InitProgressPanel* splashScreen = new InitProgressPanel();
	splashScreen->Show();

	ret = create_directory(fEditorSettings.MediaFolder().String(), 0777);
	if (ret < B_OK) {
		printf("failed to create directory \"%s\": %s\n",
			fEditorSettings.MediaFolder().String(), strerror(ret));
	}

	AttributeServerObjectManager* library
		= new AttributeServerObjectManager();
	ret = library->Init(fEditorSettings.MediaFolder().String(),
		fObjectFactory, true, splashScreen);

	fObjectLibrary = library;
	fObjectLibrary->SetLocker(fDocument);
	fObjectLibrary->SetIgnoreStateChanges(true);
		// in the editor, we want to keep the library synched ourselves
	if (ret < B_OK) {
		BString message("Failed to load object library!\n\nError: ");
		message << strerror(ret);
		BAlert* alert = new BAlert("error", message.String(), "Ignore",
			"Quit", NULL, B_WIDTH_FROM_WIDEST, B_STOP_ALERT);
		int32 choice = alert->Go();
		if (choice == 1) {
			Quit();
			return;
		}
	}

	ServerObjectManager::SetClientID(fEditorSettings.ClientID().String());

	_ValidatePlaylistLayouts();

	// try to open last edited playlist (Doing so before fMainWindow
	// is created means this is done synchronously)
	_OpenCurrentDocument();

	splashScreen->SetProgressTitle("Creating main window"B_UTF8_ELLIPSIS);

	// create and show main window
	BRect windowFrame;
	_RestoreFrameSettings(windowFrame, kWindowFrameKey,
		BRect(50.0, 50.0, 1000.0, 700.0));

	fMainWindow = new MainWindow(windowFrame, this, fDocument);
	fMainWindow->RestoreSettings(&fSettings);

	splashScreen->SetProgressTitle("Building list"B_UTF8_ELLIPSIS);

	fMainWindow->SetObjectManager(fObjectLibrary);

	splashScreen->PostMessage(B_QUIT_REQUESTED);

	fMainWindow->Show();

	// create render settings window
	fRenderSettingsWindow = new RenderSettingsWindow(
		new BMessage(MSG_RENDER_SETTINGS_ACQUIRED), this);

	SetPulseRate(1000000);

	_InstallMimeType();
}

// RefsReceived
void
EditorApp::RefsReceived(BMessage* message)
{
	BList newClips;
	BList oldClips;

	// iterate over entry_refs in the message and create clips
	entry_ref ref;
	for (int32 i = 0; message->FindRef("refs", i, &ref) >= B_OK; i++) {
		AutoReadLocker locker(fDocument);
		ServerObject* clip = _FindFileBasedClip(ref);
		if (clip) {
			clip->Acquire();
			if (!oldClips.AddItem((void*)clip)) {
				print_error("EditorApp::RefsReceived() - "
							"no memory to add clip\n");
				clip->Release();
				break;
			}
			continue;
		}
		locker.Unlock();

		status_t error;
		clip = FileBasedClip::CreateClip(fObjectLibrary, &ref, error, true);
			// also imports the file into the object library
		if (clip) {
			if (!newClips.AddItem((void*)clip)) {
				print_error("EditorApp::RefsReceived() - "
					"no memory to add clip\n");
				delete clip;
				break;
			}
		} else {
			// TODO: report to user?
			print_error("EditorApp::RefsReceived() - "
				"failed to create clip for %s: %s\n", ref.name,
				strerror(error));
		}
	}

	// add the clips to the library using AddObjectsCommand
	if (newClips.CountItems() > 0) {
// TODO: If objects running in the MainWindow thread were all using
// asynchronous listeners, this would not be necessary: Make sure we
// have the window lock or else we could dead lock if the window was
// already trying to read/write lock while the command executes.
BAutolock _(fMainWindow);
		fDocument->CommandStack()->Perform(
			new (nothrow) AddObjectsCommand(fObjectLibrary,
								  (ServerObject**)newClips.Items(),
								  newClips.CountItems(),
								  fDocument->ClipSelection()));
	}
	// make sure old clips are updated
	int32 count = oldClips.CountItems();
	if (count > 0) {
		_ReloadFileBasedClips(oldClips);

		for (int32 i = 0; i < count; i++) {
			ServerObject* clip = (ServerObject*)oldClips.ItemAtFast(i);
			clip->Release();
		}
	}
}

// Pulse
void
EditorApp::Pulse()
{
}

// #pragma mark -

// _SynchronizeClipsOnDisk
EditorApp::user_save_choice
EditorApp::_SynchronizeClipsOnDisk(bool reloadFromDisk, bool askUser,
	bool isQuit)
{
	// find out if the undo history is "saved", if so, pretend the
	// user saved
	if (fObjectLibrary->IsStateSaved() && fDocument->CommandStack()->IsSaved())
		return USER_SAVED;

	int32 choice = 2;
		// do save

	if (askUser) {
		// NOTE: reloadFromDisk would only be false when quitting
		// the application
		BAlert* alert = new BAlert("save", "Do you want to save the current "
			"playlist now and any changed clips?",
			isQuit ? "Quit" : "Don't Save", "Cancel", "Save",
			B_WIDTH_AS_USUAL, B_WARNING_ALERT);
	
		choice = alert->Go();
	}
		
	if (choice == 0) {
		// don't save
		// we still need to synchronize the clips
		// from disk, so reload all the clips and
		// dump any meanwhile added clips
		if (reloadFromDisk) {
			// detach all the windows from any currently edited objects
			_PrepareAllWindowsForSynchronization();
			// this is supposed to bring back the application's state
			// to the on-disk state of the object library
			_ReInitClipsFromDisk();
		}
		return USER_DIDNT_SAVE;
	}  else if (choice == 1) {
		// cancel operation
		return USER_CANCELED;
	} else {
		// see if the current playlist has been modified
		Playlist* playlist = fDocument->Playlist();
		if (playlist && !fDocument->CommandStack()->IsSaved())
			playlist->SetDataSaved(false);

		// only saves the actually changed clips
		print_info("saving library (state: %d, command stack: %d)\n",
			fObjectLibrary->IsStateSaved(),
			fDocument->CommandStack()->IsSaved());

		// this saves all changed objects, which we write ourself
		status_t status = fObjectLibrary->FlushObjects(fObjectFactory);
		if (status < B_OK) {
			print_error("EditorApp::QuitRequested() - FlushObjects(): %s\n",
						strerror(status));
		} else {
			fDocument->CommandStack()->Save();
		}
		// save the meta data _after_ having flushed objects (they
		// might have modified their sync status)
		fObjectLibrary->SetIgnoreStateChanges(false);
		fObjectLibrary->StateChanged();
		fObjectLibrary->SetIgnoreStateChanges(true);
		return USER_SAVED;
	}
}

// _OpenPlaylist
status_t
EditorApp::_OpenPlaylist(const BString& serverID)
{
	if (serverID.Length() <= 0)
		return B_BAD_VALUE;

	if (fMainWindow) {
		BMessage message(MSG_SET_PLAYLIST);
		message.AddString("playlist", serverID);
		fMainWindow->PostMessage(&message);
	} else {
		// NOTE: document and object library share the same lock now
		AutoWriteLocker locker(fDocument);
		if (!locker.IsLocked())
			return B_ERROR;

		Playlist* playlist = dynamic_cast<Playlist*>(
			fObjectLibrary->FindObject(serverID));

		if (!playlist) {
			print_error("EditorApp::_OpenPlaylist() - "
						"playlist %s not found in object lib\n",
						serverID.String());
			return B_BAD_VALUE;
		}

		fDocument->MakeEmpty();
		fDocument->SetPlaylist(playlist);
	}

	// TODO: not save anymore in case we use the mainwindow,
	// since we don't know if the window actually loaded the playlist ok
	fDocumentID = serverID;

	return B_OK;
}

// _OpenCurrentDocument
status_t
EditorApp::_OpenCurrentDocument()
{
	return _OpenPlaylist(fDocumentID);
}

// _NewDocument
status_t
EditorApp::_NewDocument()
{
	// write lock server object manager
	AutoWriteLocker locker(fObjectLibrary->Locker());
	
	if (!locker.IsLocked())
		RETURN_ERROR(B_ERROR);

	// create a new, clean playlist
	Playlist* playlist = new (nothrow) Playlist();
	if (!playlist)
		RETURN_ERROR(B_NO_MEMORY);

	playlist->SetName("New Playlist");

	// register the document with the object manager
	if (!fObjectLibrary->AddObject(playlist)) {
		delete playlist;
		RETURN_ERROR(B_NO_MEMORY);
	}

	return _OpenPlaylist(playlist->ID());
}

// _SaveDocument
status_t
EditorApp::_SaveDocument(Document* document,
						 const entry_ref* docRef) const
{
	if (!document)
		return B_BAD_VALUE;

	status_t ret = fObjectFactory->StoreObject(document->Playlist(),
		fObjectLibrary);

	if (ret < B_OK) {
		// inform user of failure
		BString helper("Saving your document failed!");
		helper << "\n\n" << "Error: " << strerror(ret);
		BAlert* alert = new BAlert("bad news", helper.String(),
								   "Bleep!", NULL, NULL);
		// launch alert asynchronously
		alert->Go(NULL);
	} else {
		// success, mark undo stack as saved,
		// add to global recent document list
		// update export entry_ref
//		be_roster->AddToRecentDocuments(docRef);
		document->CommandStack()->Save();
//		document->SetRef(*docRef);
	}

	return ret;
}

// _ResolveDependencies
void
EditorApp::_ResolveDependencies()
{
	_PrepareAllWindowsForSynchronization();

	InitProgressPanel* splashScreen = new InitProgressPanel();
	splashScreen->Show();

	fObjectLibrary->ResolveDependencies(splashScreen);

	fMainWindow->PostMessage(MSG_REATTACH);

	splashScreen->PostMessage(B_QUIT_REQUESTED);
}

// #pragma mark - networking

#ifndef CLOCKWERK_STAND_ALONE

// _Synchronize
status_t
EditorApp::_Synchronize()
{
	if (!fConnection)
		RETURN_ERROR(B_NO_INIT);
	if (fSynchronizer)
		RETURN_ERROR(B_BUSY);

	_PrepareAllWindowsForSynchronization();

	fNetworkStatusPanel->PostMessage(MSG_UPDATING);

	user_save_choice choice = _SynchronizeClipsOnDisk(false);
	if (choice != USER_SAVED) {
		if (choice == USER_DIDNT_SAVE) {
			BAlert* alert = new BAlert("info", "The local object "
				"library needs to be saved before it can be "
				"synchronized with the server.", "Understood");
			alert->Go();
		}
		_NotifyNetworkJobDone();
		return B_CANCELED;
	}

	// start the synchronization
	fSynchronizer = new Synchronizer(fConnection, fObjectLibrary,
		fObjectFactory, "", true, false, fNetworkStatusPanel->StatusOutput());

	status_t error = fSynchronizer->Init();

	if (error < B_OK) {
		_NotifyNetworkJobDone();
		RETURN_ERROR(error);
	}

	AddHandler(fSynchronizer);

	fObjectLibrary->SetIgnoreStateChanges(false);

	error = fSynchronizer->Update();
	if (error != B_OK)
		fObjectLibrary->SetIgnoreStateChanges(true);
	return error;
}

// _Synchronized
status_t
EditorApp::_Synchronized(bool complete, bool canceled)
{
	RemoveHandler(fSynchronizer);
	delete fSynchronizer;
	fSynchronizer = NULL;

	// ignore state changes again
	fObjectLibrary->SetIgnoreStateChanges(true);

	BString summary;
	if (!complete)
		summary << "Update finished, errors occured - check log!";
	else
		summary << "Update finished without errors.";
	if (canceled)
		summary << " User canceled.";
	summary << '\n';

	if (complete && !canceled) {
		// remove objects that are SERVER_REMOVED
		CleanupObsoleteObjects();
	}

	fNetworkStatusPanel->StatusOutput()->PrintInfoMessage(
		summary.String());
	_NotifyNetworkJobDone();

	status_t ret = fObjectLibrary->ResolveDependencies();;

	_SynchronizeClipsOnDisk(false, false);

	return ret;
}

// _Upload
status_t
EditorApp::_Upload()
{
	if (!fConnection)
		RETURN_ERROR(B_NO_INIT);
	if (fUploader)
		RETURN_ERROR(B_BUSY);

	_PrepareAllWindowsForSynchronization();

	fNetworkStatusPanel->PostMessage(MSG_COMMITING);

	user_save_choice choice = _SynchronizeClipsOnDisk(false);
	if (choice != USER_SAVED) {
		if (choice == USER_DIDNT_SAVE) {
			BAlert* alert = new BAlert("info", "The local object "
				"library needs to be saved before it can be "
				"synchronized with the server.", "Understood");
			alert->Go();
		}
		_NotifyNetworkJobDone();
		return B_CANCELED;
	}

	// start the synchronization
	Uploader* uploader = new Uploader(fConnection, fObjectLibrary,
		fNetworkStatusPanel->StatusOutput());

	ObjectDeleter<Uploader> deleter(uploader);

	status_t error = uploader->Init();

	if (error < B_OK) {
		_NotifyNetworkJobDone();
		RETURN_ERROR(error);
	}

	error = uploader->Collect();
	if (error < B_OK) {
		_NotifyNetworkJobDone();
		RETURN_ERROR(error);
	}

	// confront user with a panel showing all to be uploaded objects
	UploadSelectionPanel* panel = new UploadSelectionPanel(
		fSettings, fMainWindow);
	error = panel->InitFromUploader(uploader);
	if (error < B_OK) {
		_NotifyNetworkJobDone();
		RETURN_ERROR(error);
	}

	// allow user to cancel or continue the upload
	UploadSelectionPanel::return_code code = panel->Go(true);
	if (code == UploadSelectionPanel::RETURN_CANCEL) {
		_NotifyNetworkJobDone();
		return B_OK;
	}

	deleter.Detach();
	fUploader = uploader;
	AddHandler(fUploader);

	return fUploader->Upload();
}

// _Uploaded
status_t
EditorApp::_Uploaded(bool complete)
{
	RemoveHandler(fUploader);
	delete fUploader;
	fUploader = NULL;

	if (complete) {
		fNetworkStatusPanel->StatusOutput()->PrintInfoMessage(
			"Commit complete.\n");
	} else {
		fNetworkStatusPanel->StatusOutput()->PrintErrorMessage(
			"Commit failed.\n");
	}
	_NotifyNetworkJobDone();

	status_t ret = fObjectLibrary->ResolveDependencies();

	_SynchronizeClipsOnDisk(false, false);

	return ret;
}

// CleanupObsoleteObjects
void
EditorApp::CleanupObsoleteObjects()
{
	AutoWriteLocker _(fObjectLibrary->Locker());
	int32 count = fObjectLibrary->CountObjects();
	int32 obsoleteObjects = 0;
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fObjectLibrary->ObjectAtFast(i);
		if (object->Status() == SYNC_STATUS_SERVER_REMOVED)
			obsoleteObjects++;
	}
	if (obsoleteObjects == 0)
		return;

	// TODO: present a listview with the obsolete objects
	BString message;
	if (obsoleteObjects > 1) {
		message << obsoleteObjects << " objects have been removed on ";
		message << "the server. ";
		message << "Should these objects be removed from this computer ";
		message << "now? This object could still be referenced by ";
		message << "other objects.";
	} else {
		message << "One object has been removed on the server. ";
		message << "Should this object be removed from this computer ";
		message << "now? Some of these objects could still be referenced ";
		message << "by other objects.";
	}
	message << "All removed objects are still archived on the ";
	message << "server and could be restored later.";

	BAlert* alert = new BAlert("obsolete confirmation", message.String(),
		"Keep", "Remove", NULL, B_WIDTH_FROM_WIDEST, B_WARNING_ALERT);
	int32 choice = alert->Go();
	if (choice != 1)
		return;

	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fObjectLibrary->ObjectAtFast(i);
		if (object->Status() != SYNC_STATUS_SERVER_REMOVED)
			continue;
		// remove object from library
		if (!fObjectLibrary->RemoveObject(object)) {
			print_error("unable to remove 'removed' object '%s' "
			"(%s, t %s, v %ld) from library!\n",
			object->ID().String(), object->Name().String(),
			object->Type().String(), object->Version());
			continue;
		}
		// do not skip the next object
		i--;
		// one object less
		count--;
		// remove file
		entry_ref ref;
		if (fObjectLibrary->GetRef(object, ref) == B_OK) {
			BEntry entry(&ref);
			if (entry.Exists()) {
				status_t removeStatus = entry.Remove();
				if (removeStatus != B_OK) {
					print_error("unable to remove file for 'removed' "
					"object '%s' (%s, t %s, v %ld) from library: %s\n",
					object->ID().String(), object->Name().String(),
					object->Type().String(), object->Version(),
					strerror(removeStatus));
				}
			}
		}
		// no more need for this object
		object->Release();
	}
}

#endif // CLOCKWERK_STAND_ALONE

// _HandleScopes
void
EditorApp::_HandleScopes(BMessage* scopes)
{
	print_info("received \"scopes\" from server\n");
	fScopes = *scopes;

#ifndef CLOCKWERK_STAND_ALONE
	if (fClientSettingsWindow->Lock()) {
		fClientSettingsWindow->SetScopes(&fScopes);
		fClientSettingsWindow->Unlock();
	}

	if (fScheduleWindow->Lock()) {
		fScheduleWindow->SetScopes(&fScopes);
		fScheduleWindow->Unlock();
	}

	if (fDisplaySettingsWindow->Lock()) {
		fDisplaySettingsWindow->SetScopes(&fScopes);
		fDisplaySettingsWindow->Unlock();
	}
#endif // CLOCKWERK_STAND_ALONE
}

// #pragma mark -

// _ShowOpenPanel
void
EditorApp::_ShowOpenPanel()
{
	if (!fOpenPanel) {
		// create panel
		fOpenPanel = new BFilePanel(B_OPEN_PANEL,
									&be_app_messenger,
									NULL,	// panel folder
									0,		// node flavors
									false,	// multiple selection
									new BMessage(MSG_OPEN));
	} else {
		// refresh folder contents before showing
		fOpenPanel->Refresh();
	}
	// copy position & folder from open panel
	if (fLastPanel && fLastPanel != fOpenPanel) {
		// TODO: ...
		fLastPanel = fOpenPanel;
	}

	fOpenPanel->Show();
}

// _ShowSavePanel
void
EditorApp::_ShowSavePanel(BMessage* message,
						  const char* title, const char* label)
{
	if (!fSavePanel) {
		// create panel
		fSavePanel = new BFilePanel(B_SAVE_PANEL,
									&be_app_messenger,
									NULL,	// panel folder
									0,		// node flavors
									false,	// multiple selection
									NULL);
	} else {
		// refresh folder contents before showing
		fSavePanel->Refresh();
	}
	// copy position & folder from open panel
	if (fLastPanel && fLastPanel != fSavePanel) {
		// TODO: ...
		fLastPanel = fSavePanel;
	}

	if (fSavePanel->Window()->Lock()) {
		BString windowTitle("Clockwerk: ");
		if (title)
			windowTitle << title;
		else
			windowTitle << "Save";
		fSavePanel->Window()->SetTitle(windowTitle.String());
		fSavePanel->Window()->Unlock();
	}
	if (message)
		fSavePanel->SetMessage(message);
	if (label)
		fSavePanel->SetButtonLabel(B_DEFAULT_BUTTON, label);
	else
		fSavePanel->SetButtonLabel(B_DEFAULT_BUTTON, "Save");

	fSavePanel->Show();
}

// _GetRefFromFilePanelMessage
status_t
EditorApp::_GetRefFromFilePanelMessage(BMessage* message, entry_ref* ref) const
{
	status_t ret = message->FindRef("directory", ref);
	if (ret < B_OK)
		return ret;

	const char* name;
	ret = message->FindString("name", &name);
	if (ret < B_OK)
		return ret;

	BDirectory dir(ref);
	ret = dir.InitCheck();
	if (ret < B_OK) {
		print_error("EditorApp::_GetRefFromFilePanelMessage() - "
					"dir.InitCheck(): %s\n", strerror(ret));
		return ret;
	}

	BEntry entry;
	ret = entry.SetTo(&dir, name, true);
	if (ret < B_OK) {
		printf("EditorApp::_GetRefFromFilePanelMessage() - "
			   "entry.SetTo(): %s\n", strerror(ret));
		return ret;
	}

	ret = entry.GetRef(ref);
	if (ret < B_OK) {
		print_error("EditorApp::_GetRefFromFilePanelMessage() - "
					"entry.GetRef(): %s\n", strerror(ret));
		return ret;
	}

	return B_OK;
}

// #pragma mark -

// _LoadSettings
void
EditorApp::_LoadSettings()
{
	status_t ret = load_settings(&fSettings, "clockwerk_ui_settings",
		kM3BaseSettingsName);
	if (ret < B_OK)
		return;

	fSettings.FindString(kCurrentDocKey, &fDocumentID);

	if (fSettings.FindMessage(kScopesKey, &fScopes) < B_OK) {
		// default scope
		fScopes.MakeEmpty();
		fScopes.AddString("scop", "all");
	}
}

// _SaveSettings
void
EditorApp::_SaveSettings()
{
	if (fSettings.ReplaceString(kCurrentDocKey, fDocumentID.String()) < B_OK)
		fSettings.AddString(kCurrentDocKey, fDocumentID.String());

	if (fSettings.ReplaceMessage(kScopesKey, &fScopes) < B_OK)
		fSettings.AddMessage(kScopesKey, &fScopes);

	if (fMainWindow->Lock()) {
		if (fSettings.ReplaceRect(kWindowFrameKey, fMainWindow->Frame()) < B_OK)
			fSettings.AddRect(kWindowFrameKey, fMainWindow->Frame());
		fMainWindow->StoreSettings(&fSettings);
		fMainWindow->Unlock();
	}

#ifndef CLOCKWERK_STAND_ALONE
	_SaveFrameSettings(fScheduleWindow, kScheduleWindowFrameKey);
	_SaveFrameSettings(fClientSettingsWindow, kClientSettingsWindowFrameKey);
	_SaveFrameSettings(fUserWindow, kUserWindowFrameKey);
	_SaveFrameSettings(fDisplaySettingsWindow, kDisplaySettingsWindowFrameKey);
	_SaveFrameSettings(fNetworkStatusPanel, kNetworkStatusWindowFrameKey);

	if (fScheduleWindow->Lock()) {
		fScheduleWindow->StoreSettings(&fSettings);
		fScheduleWindow->Unlock();
	}
#endif

	status_t ret = save_settings(&fSettings, "clockwerk_ui_settings",
		kM3BaseSettingsName);
	if (ret < B_OK)
		print_error("unable to save settings: %s\n", strerror(ret));
}

// _RestoreFrameSettings
void
EditorApp::_RestoreFrameSettings(BRect& frame, const char* key,
	BRect defaultFrame, BWindow* window) const
{
	if (fSettings.FindRect(key, &frame) < B_OK)
		frame = defaultFrame;
	make_sure_frame_is_on_screen(frame, window);
}

// _SaveFrameSettings
void
EditorApp::_SaveFrameSettings(BWindow* window, const char* key)
{
	if (window->Lock()) {
		if (fSettings.ReplaceRect(key, window->Frame()) < B_OK)
			fSettings.AddRect(key, window->Frame());
		window->Unlock();
	}
}

// _ShowWindow
void
EditorApp::_ShowWindow(BWindow* window)
{
	if (!window->Lock())
		return;

	if (window->IsHidden())
		window->Show();
	else
		window->Activate();

	window->Unlock();
}

// _QuitWindow
template<class SomeWindow>
void
EditorApp::_QuitWindow(SomeWindow** window)
{
	if ((*window)->Lock()) {
		(*window)->Quit();
		*window = NULL;
	}
}

// _PrepareAllWindowsForSynchronization
void
EditorApp::_PrepareAllWindowsForSynchronization()
{
	for (int32 i = 0; BWindow* window = WindowAt(i); i++) {
#ifndef CLOCKWERK_STAND_ALONE
		if (window == fNetworkStatusPanel)
			continue;
#endif
		if (window == fRenderSettingsWindow)
			continue;
		_PrepareForSynchronization(window);
	}
}

// _PrepareForSynchronization
void
EditorApp::_PrepareForSynchronization(BWindow* window)
{
	BMessenger messenger(window);
	if (!messenger.IsValid()) {
		print_error("window messenger invalid when trying to "
			"prepare for sync\n");
		return;
	}

	BMessage reply;
	status_t ret = messenger.SendMessage(MSG_PREPARE_FOR_SYNCHRONIZATION,
		&reply);
	if (ret < B_OK) {
		print_error("error sending 'prepare for sync' command to "
			"window '%s': %s\n", window->Title(), strerror(ret));
	}

	if (reply.what != MSG_READY_FOR_SYNCHRONIZATION) {
		print_warning("unexpected reply for 'prepare for sync' command from "
			"window '%s'\n", window->Title());
	}
}

// #pragma mark -

// _FindFileBasedClip
ServerObject*
EditorApp::_FindFileBasedClip(const entry_ref& ref) const
{
	int32 count = fObjectLibrary->CountObjects();
	for (int32 i = 0; i < count; i++) {
		FileBasedClip* clip = dynamic_cast<FileBasedClip*>(
			fObjectLibrary->ObjectAtFast(i));
		if (!clip)
			continue;
		if (clip->Ref() && *clip->Ref() == ref)
			return clip;
	}
	return NULL;
}

// _ReloadFileBasedClips
void
EditorApp::_ReloadFileBasedClips(const BList& clips)
{
	bool mainWindowLocked = fMainWindow && fMainWindow->Lock();

	AutoWriteLocker locker(fDocument);
	if (!locker.IsLocked())
		return;

	int32 count = clips.CountItems();
	for (int32 i = 0; i < count; i++) {
		FileBasedClip* clip = dynamic_cast<FileBasedClip*>(
			(ServerObject*)clips.ItemAtFast(i));
		if (!clip) {
			// not actually possible
			continue;
		}
		clip->Reload();
	}

	if (mainWindowLocked)
		fMainWindow->Unlock();
}

// _ResetAllObjectsToLocal
void
EditorApp::_ResetAllObjectsToLocal()
{
	BAutolock _(fMainWindow);

	AutoWriteLocker locker(fObjectLibrary->Locker());
	if (!locker.IsLocked())
		return;

	int32 count = fObjectLibrary->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fObjectLibrary->ObjectAtFast(i);
		object->SetStatus(SYNC_STATUS_LOCAL);
		object->SetVersion(0);
	}
}

// _InstallMimeType
void
EditorApp::_InstallMimeType()
{
	// install mime type of documents
	BMimeType mime(kNativeDocumentMimeType);
	status_t ret = mime.InitCheck();
	if (ret < B_OK) {
		fprintf(stderr, "Could not init native document mime type (%s): %s.\n",
			kNativeDocumentMimeType, strerror(ret));
		return;
	}

	if (mime.IsInstalled() && !(modifiers() & B_SHIFT_KEY)) {
		// mime is already installed, and the user is not
		// pressing the shift key to force a re-install
		return;
	}

	ret = mime.Install();
	if (ret < B_OK) {
		fprintf(stderr, "Could not install native document mime type (%s): %s.\n",
			kNativeDocumentMimeType, strerror(ret));
		return;
	}
	// set preferred app
	ret = mime.SetPreferredApp(kClockwerkMimeSig);
	if (ret < B_OK)
		fprintf(stderr, "Could not set native document preferred app: %s\n",
			strerror(ret));

	// set descriptions
	ret = mime.SetShortDescription("Clockwerk Object");
	if (ret < B_OK)
		fprintf(stderr, "Could not set short description of mime type: %s\n",
			strerror(ret));
	ret = mime.SetLongDescription("Clockwerk Object File");
	if (ret < B_OK)
		fprintf(stderr, "Could not set long description of mime type: %s\n",
			strerror(ret));

	// attribute infos
	BMessage attrMessage;

	// name
	attrMessage.AddString("attr:name", "CLKW:name");
	attrMessage.AddString("attr:public_name", "Name");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", true);
	attrMessage.AddInt32("attr:width", 120);
	attrMessage.AddInt32("attr:alignment", 0);

	// type
	attrMessage.AddString("attr:name", "CLKW:type");
	attrMessage.AddString("attr:public_name", "Type");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", false);
	attrMessage.AddInt32("attr:width", 120);
	attrMessage.AddInt32("attr:alignment", 0);

	// version
	attrMessage.AddString("attr:name", "CLKW:vrsn");
	attrMessage.AddString("attr:public_name", "Version");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", false);
	attrMessage.AddInt32("attr:width", 60);
	attrMessage.AddInt32("attr:alignment", 1);

	// sync status
	attrMessage.AddString("attr:name", "CLKW:syst");
	attrMessage.AddString("attr:public_name", "Sync Status");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", false);
	attrMessage.AddInt32("attr:width", 80);
	attrMessage.AddInt32("attr:alignment", 0);

	// description
	attrMessage.AddString("attr:name", "CLKW:dscr");
	attrMessage.AddString("attr:public_name", "Description");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", false);
	attrMessage.AddInt32("attr:width", 120);
	attrMessage.AddInt32("attr:alignment", 0);

	// scope
	attrMessage.AddString("attr:name", "CLKW:scop");
	attrMessage.AddString("attr:public_name", "Scope");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", false);
	attrMessage.AddInt32("attr:width", 80);
	attrMessage.AddInt32("attr:alignment", 0);

	// keywords
	attrMessage.AddString("attr:name", "CLKW:kwrd");
	attrMessage.AddString("attr:public_name", "Keywords");
	attrMessage.AddInt32("attr:type", B_STRING_TYPE);
	attrMessage.AddBool("attr:viewable", true);
	attrMessage.AddBool("attr:editable", true);
	attrMessage.AddInt32("attr:width", 100);
	attrMessage.AddInt32("attr:alignment", 0);

	ret = mime.SetAttrInfo(&attrMessage);
	if (ret < B_OK)
		fprintf(stderr, "Could not set extra attribute description "
			"of mime type: %s\n", strerror(ret));
}




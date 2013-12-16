/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EDITOR_APP_H
#define EDITOR_APP_H

#include <Message.h>
#include <String.h>

#include "common_constants.h"

#include "ClockwerkApp.h"
#include "EditorSettings.h"

class BFilePanel;
class ClientSettingsWindow;
class DisplaySettingsWindow;
class ErrorLogWindow;
class MainWindow;
class NetworkStatusPanel;
class PersistentServerObjectManager;
class RenderSettingsWindow;
class ScheduleWindow;
class ServerObject;
class StatusOutput;
class Synchronizer;
class Uploader;
class UserWindow;

enum {
	MSG_NEW							= 'newd',
	MSG_SAVE						= 'save',

	MSG_RENDER_PLAYLIST				= 'rndr',

	MSG_PREPARE_FOR_SYNCHRONIZATION	= 'nwps',
	MSG_READY_FOR_SYNCHRONIZATION	= 'nwrs',

	MSG_CREATE_OBJECT				= 'crto',
	MSG_RESOLVE_DEPENDENCIES		= 'rslv'
};

class EditorApp : public ClockwerkApp {
 public:
								EditorApp();
	virtual						~EditorApp();

	// BApplication interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				Pulse();

 private:

			void				_HandleScopes(BMessage* scopes);

			typedef enum {
				USER_CANCELED	= 0,
				USER_SAVED		= 1,
				USER_DIDNT_SAVE	= 2,
			} user_save_choice;

			user_save_choice	_SynchronizeClipsOnDisk(bool reloadFromDisk,
														bool askUser = true,
														bool isQuit = false);

			status_t			_OpenPlaylist(const BString& serverID);

			status_t			_OpenCurrentDocument();

			status_t			_NewDocument();
			status_t			_SaveDocument(Document* document,
											  const entry_ref* ref) const;
			void				_ResolveDependencies();

			void				_ShowOpenPanel();
			void				_ShowSavePanel(BMessage* message = NULL,
											   const char* title = NULL,
											   const char* label = NULL);
			status_t			_GetRefFromFilePanelMessage(
									BMessage* message, entry_ref* ref) const;

			void				_LoadSettings();
			void				_SaveSettings();

			void				_RestoreFrameSettings(BRect& frame,
									const char* key, BRect defaultFrame,
									BWindow* window = NULL) const;
			void				_SaveFrameSettings(BWindow* window,
									const char* key);

			void				_ShowWindow(BWindow* window);
			template<class SomeWindow>
			void				_QuitWindow(SomeWindow** window);
			void				_PrepareAllWindowsForSynchronization();
			void				_PrepareForSynchronization(BWindow* window);

			ServerObject*		_FindFileBasedClip(const entry_ref& ref) const;
			void				_ReloadFileBasedClips(const BList& clips);

			void				_ResetAllObjectsToLocal();

			void				_InstallMimeType();

	EditorSettings				fEditorSettings;

	MainWindow*					fMainWindow;
	RenderSettingsWindow*		fRenderSettingsWindow;
	ErrorLogWindow*				fErrorLogWindow;

	BFilePanel*					fOpenPanel;
	BFilePanel*					fSavePanel;
	BFilePanel*					fLastPanel;

	BMessage					fSettings;
	BMessage					fScopes;
	BString						fDocumentID;
};

#endif // EDITOR_APP_H

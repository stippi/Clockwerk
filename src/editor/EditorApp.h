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
#include "JobConnection.h"

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

#ifndef CLOCKWERK_STAND_ALONE
	MSG_SHOW_SCHEDULES				= 'shsc',
	MSG_SHOW_CLIENT_SETTINGS		= 'shcs',
	MSG_SHOW_USERS					= 'shus',
	MSG_SHOW_DISPLAY_SETTINGS		= 'shds',

	MSG_NETWORK_SHOW_STATUS			= 'nwss',
	MSG_NETWORK_HIDE_STATUS			= 'nwhs',

	MSG_NETWORK_RESET_ALL_OBJECTS	= 'nwro',
#endif

	MSG_CREATE_OBJECT				= 'crto',
	MSG_RESOLVE_DEPENDENCIES		= 'rslv'
};

class EditorApp : public ClockwerkApp
#ifndef CLOCKWERK_STAND_ALONE
				, public JobConnectionListener
#endif
				{
 public:
								EditorApp();
	virtual						~EditorApp();

	// BApplication interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				ReadyToRun();
	virtual	void				RefsReceived(BMessage* message);
	virtual	void				Pulse();

#ifndef CLOCKWERK_STAND_ALONE
	// JobConnectionListener interface
	virtual	void				Connecting(JobConnection* connection);
	virtual	void				Connected(JobConnection* connection);
	virtual	void				Disconnected(JobConnection* connection);
	virtual	void				Deleted(JobConnection* connection);
#endif // CLOCKWERK_STAND_ALONE

 private:
#ifndef CLOCKWERK_STAND_ALONE
			status_t			_Synchronize();
			status_t			_Synchronized(bool complete = true,
									bool canceled = false);

			status_t			_Upload();
			status_t			_Uploaded(bool complete = true);

			void				CleanupObsoleteObjects();
#endif // CLOCKWERK_STAND_ALONE

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

#ifndef CLOCKWERK_STAND_ALONE
			void				_CleanupNetworkStatusForNewConnection();
			void				_DeleteConnection();
			void				_AutoShowNetworkStatusWindow(
									const char* initialStatus);
			void				_AutoHideNetworkStatusWindow();
#endif // CLOCKWERK_STAND_ALONE
			void				_NotifyNetworkJobDone();

			void				_InstallMimeType();

	EditorSettings				fEditorSettings;

	MainWindow*					fMainWindow;
#ifndef CLOCKWERK_STAND_ALONE
	ScheduleWindow*				fScheduleWindow;
	ClientSettingsWindow*		fClientSettingsWindow;
	UserWindow*					fUserWindow;
	DisplaySettingsWindow*		fDisplaySettingsWindow;
	NetworkStatusPanel*			fNetworkStatusPanel;
#endif
	RenderSettingsWindow*		fRenderSettingsWindow;
	ErrorLogWindow*				fErrorLogWindow;

	BFilePanel*					fOpenPanel;
	BFilePanel*					fSavePanel;
	BFilePanel*					fLastPanel;

#ifndef CLOCKWERK_STAND_ALONE
	JobConnection*				fConnection;
	Synchronizer*				fSynchronizer;
	Uploader*					fUploader;
#endif

	BMessage					fSettings;
	BMessage					fScopes;
	BString						fDocumentID;
};

#endif // EDITOR_APP_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIENT_SETTINGS_WINDOW_H
#define CLIENT_SETTINGS_WINDOW_H

#include <Window.h>

#include "Observer.h"
#include "ServerObjectManager.h"

class BCheckBox;
class BMenu;
class BMenuField;
class BMenuItem;
class BTextControl;
class ClientSettingsListView;
class EditorApp;
class Group;

//enum {
//	MSG_			= '',
//};

class ClientSettingsWindow : public BWindow,
							 public Observer,
							 public SOMListener {
 public:
								ClientSettingsWindow(BRect frame,
													 EditorApp* app);
	virtual						~ClientSettingsWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// SOMListener interface
	virtual	void				ObjectAdded(ServerObject* object,
											int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

	// ClientSettingsWindow
			void				SetObjectManager(
									ServerObjectManager* manager);

			void				SetCurrentSettings(ServerObject* object);
			void				SetScopes(const BMessage* scopes);

 private:
			void				_Init();
			void				_CreateGUI();
			BMenuBar*			_CreateMenuBar();

			void				_UpdateSettings();

	// we need the application for some stuff
	EditorApp*					fApp;
	// object manager
	ServerObjectManager*		fObjectManager;
	// client settings
	ServerObject*				fCurrentSettings;

	// views
	BMenuBar*					fMenuBar;
	BMenu*						fClientSettingsMenu;
	BMenuItem*					fCreateMI;

	ClientSettingsListView*		fListView;

	BTextControl*				fSettingsName;
	BMenuField*					fSettingsScope;

	BTextControl*				fServerAddress1;
	BTextControl*				fServerAddress2;
	BCheckBox*					fEraseObjects;
	BCheckBox*					fEraseRevisions;
	BTextControl*				fLogUploadTime;
};

#endif // CLIENT_SETTINGS_WINDOW_H

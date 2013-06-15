/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DISPLAY_SETTINGS_WINDOW_H
#define DISPLAY_SETTINGS_WINDOW_H

#include <Window.h>

#include "Observer.h"
#include "ServerObjectManager.h"

class BMenu;
class BMenuField;
class BMenuItem;
class BTextControl;
class DisplaySettingsListView;
class EditorApp;
class Group;

//enum {
//	MSG_			= '',
//};

class DisplaySettingsWindow : public BWindow,
							  public Observer,
							  public SOMListener {
 public:
								DisplaySettingsWindow(BRect frame,
									EditorApp* app);
	virtual						~DisplaySettingsWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// SOMListener interface
	virtual	void				ObjectAdded(ServerObject* object,
											int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

	// DisplaySettingsWindow
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

	DisplaySettingsListView*	fListView;

	BTextControl*				fSettingsName;
	BMenuField*					fSettingsScope;
	BTextControl*				fSettingsWidth;
	BTextControl*				fSettingsHeight;
};

#endif // DISPLAY_SETTINGS_WINDOW_H

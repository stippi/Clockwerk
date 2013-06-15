/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef USER_WINDOW_H
#define USER_WINDOW_H

#include <Window.h>

#include "Observer.h"
#include "ServerObjectManager.h"

class BMenu;
class BMenuItem;
class BTextControl;
class UserListView;
class EditorApp;
class Group;

//enum {
//	MSG_			= '',
//};

class UserWindow : public BWindow,
				   public Observer,
				   public SOMListener {
 public:
								UserWindow(BRect frame, EditorApp* app);
	virtual						~UserWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// SOMListener interface
	virtual	void				ObjectAdded(ServerObject* object,
											int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

	// UserWindow
			void				SetObjectManager(
									ServerObjectManager* manager);

			void				SetCurrentUser(ServerObject* object);

 private:
			void				_Init();
			void				_CreateGUI(BRect frame);
			BMenuBar*			_CreateMenuBar(BRect frame);

			void				_UpdateUser();

	// we need the application for some stuff
	EditorApp*					fApp;
	// object manager
	ServerObjectManager*		fObjectManager;
	// client settings
	ServerObject*				fCurrentUser;

	// views
	BMenuBar*					fMenuBar;
	BMenu*						fUserMenu;
	BMenuItem*					fCreateMI;

	UserListView*				fListView;

	BTextControl*				fUserName;
	BTextControl*				fPassword;
};

#endif // CLIENT_SETTINGS_WINDOW_H

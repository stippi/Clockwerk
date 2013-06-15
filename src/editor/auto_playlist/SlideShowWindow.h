/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SLIDE_SHOW_WINDOW_H
#define SLIDE_SHOW_WINDOW_H

#include <Window.h>

#include "Observer.h"
#include "PlaylistObserver.h"
#include "ServerObjectManager.h"

class SlideShowPlaylist;
class BMenu;
class BMenuItem;
class BTextControl;
class CommandStack;
class PlaylistListView;
class RWLocker;

//enum {
//	MSG_			= '',
//};

class SlideShowWindow : public BWindow,
						public Observer,
						public PlaylistObserver {
 public:
								SlideShowWindow(BRect frame,
												RWLocker* locker);
	virtual						~SlideShowWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// PlaylistObserver interface
	virtual	void				NotificationBlockFinished(Playlist* playlist);

	// SlideShowWindow
			void				SetPlaylist(SlideShowPlaylist* playlist);

 private:
			void				_Init();
			void				_CreateGUI();
			BMenuBar*			_CreateMenuBar();

			void				_UpdateControls();

	SlideShowPlaylist*			fPlaylist;

	RWLocker*					fLocker;
	CommandStack*				fCommandStack;

	// views
	BMenuBar*					fMenuBar;
	BMenu*						fMenu;

	PlaylistListView*			fListView;

	BTextControl*				fTotalDuration;
	BTextControl*				fTransitionDuration;
};

#endif // SLIDE_SHOW_WINDOW_H

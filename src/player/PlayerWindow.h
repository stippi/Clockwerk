/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYER_WINDOW_H
#define PLAYER_WINDOW_H

#include <Window.h>

class BMessageRunner;
class Document;
class PlayerApp;
class PlayerPlaybackNavigator;
class PlayerVideoView;
class Playlist;
class VideoView;

class SimplePlaybackManager;

enum {
	MSG_EXIT_FULLSCREEN			= 'exfs',
	MSG_TOGGLE_FULLSCREEN		= 'flsc',
	MSG_TOGGLE_HIDE				= 'hide',
};

class PlayerWindow : public BWindow {
 public:
								PlayerWindow(BRect frame, window_feel feel,
									PlayerApp* app, Document* document,
									PlayerPlaybackNavigator* navigator,
									bool testMode,
									bool forceFullFrameRate,
									bool ignoreNoOverlay);
	virtual						~PlayerWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// PlayerWindow
			void				SetPlaylist(Playlist* playlist,
									int64 startFrameOffset);
			Playlist*			CurrentPlaylist() const;

			void				InitPlayback(int32 width, int32 height);
			void				ShutdownPlayback(bool disconnectNodes = true);

			void				GetPlaybackSize(int32* width,
												int32* height) const;

			void				StartPlaying();
			bool				IsPlaying();

			void				MediaServerLaunched();
			void				MediaServerQuit();

 private:
	// app & document
	PlayerApp*					fApp;
	Document*					fDocument;

	// playback framework
	SimplePlaybackManager*		fPlaybackManager;
	PlayerVideoView*			fVideoView;
	bool						fForceFullFrameRate;
	bool						fIgnoreNoOverlay;
};

#endif // PLAYER_WINDOW_H

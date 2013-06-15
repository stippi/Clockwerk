/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlayerWindow.h"

#include <stdio.h>

#include <MediaRoster.h>
#include <MessageRunner.h>
#include <Screen.h>

#include "common.h"

#include "Document.h"
#include "ClockwerkApp.h"
#include "MessageConstants.h"
#include "PlayerApp.h"
#include "PlayerVideoView.h"
#include "Playlist.h"
#include "PlaybackManager.h"
#include "SimplePlaybackManager.h"
#include <View.h>

enum {
	MSG_MEDIA_SERVER_LAUNCHED	= 'msln',
	MSG_MEDIA_SERVER_QUIT		= 'msqt',
};

// constructor
PlayerWindow::PlayerWindow(BRect frame, window_feel feel, PlayerApp* app,
		Document* document, PlayerPlaybackNavigator* navigator, bool testMode,
		bool forceFullFrameRate, bool ignoreNoOverlay)
	: BWindow(frame, "Clockwerk-Player",
			  B_TITLED_WINDOW_LOOK, feel, B_ASYNCHRONOUS_CONTROLS)
	, fApp(app)
	, fDocument(document)
	, fPlaybackManager(NULL)
	, fVideoView(NULL)
	, fForceFullFrameRate(forceFullFrameRate)
	, fIgnoreNoOverlay(ignoreNoOverlay)
{
	AddShortcut('H', B_COMMAND_KEY, new BMessage(MSG_TOGGLE_HIDE));
	AddShortcut('F', B_COMMAND_KEY, new BMessage(MSG_TOGGLE_FULLSCREEN));

	status_t ret = B_OK;
	BMediaRoster::Roster(&ret);

	if (!testMode)
		be_app->HideCursor();

	// create the GUI
	fVideoView = new PlayerVideoView(Bounds(), testMode);
	fVideoView->SetNavigator(navigator);
	AddChild(fVideoView);

	Show();

	snooze(100000);
	Lock();

	// create and run the playback frame work
	// TODO: do this here already?
	InitPlayback(DEFAULT_WIDTH, DEFAULT_HEIGHT);

	Unlock();
}

// destructor
PlayerWindow::~PlayerWindow()
{
}

// #pragma mark -

// MessageReceived
void
PlayerWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_TOGGLE_HIDE:
			if (IsMinimized()) {
				BScreen screen(this);
				window_feel feel;
				if (Frame() == screen.Frame())
					feel = B_FLOATING_ALL_WINDOW_FEEL;
				else
					feel = B_NORMAL_WINDOW_FEEL;
				SetFeel(feel);
				Minimize(false);
			} else {
				SetFeel(B_NORMAL_WINDOW_FEEL);
				Minimize(true);
			}
			break;
		case MSG_EXIT_FULLSCREEN:
		case MSG_TOGGLE_FULLSCREEN: {
			BScreen screen(this);
			if (Frame() == screen.Frame()) {
				print_info("user requested window mode\n");
				MoveTo(50, 50);
				int32 width, height;
				GetPlaybackSize(&width, &height);
				ResizeTo(width, height);
				SetFeel(B_NORMAL_WINDOW_FEEL);
			} else if (message->what != MSG_EXIT_FULLSCREEN) {
				print_info("user requested fullscreen mode\n");
				MoveTo(screen.Frame().LeftTop());
				ResizeTo(screen.Frame().Width(), screen.Frame().Height());
				SetFeel(B_FLOATING_ALL_WINDOW_FEEL);
			}
			break;
		}

		case MSG_MEDIA_SERVER_LAUNCHED:
			InitPlayback(DEFAULT_WIDTH, DEFAULT_HEIGHT);
			StartPlaying();
			break;
		case MSG_MEDIA_SERVER_QUIT:
			ShutdownPlayback(false);
			break;

		default:
			BWindow::MessageReceived(message);
	}
}

// QuitRequested
bool
PlayerWindow::QuitRequested()
{
	print_info("user requested quit\n");
	ShutdownPlayback();
	be_app->PostMessage(B_QUIT_REQUESTED);
	return false;
}

// #pragma mark -

// SetPlaylist
void
PlayerWindow::SetPlaylist(Playlist* playlist, int64 startFrameOffset)
{
	if (fPlaybackManager)
		fPlaybackManager->SetPlaylist(playlist, startFrameOffset);
	fVideoView->SetPlaylist(playlist);
}

// CurrentPlaylist
Playlist*
PlayerWindow::CurrentPlaylist() const
{
	if (fPlaybackManager)
		return fPlaybackManager->Playlist();
	return NULL;
}

// InitPlayback
void
PlayerWindow::InitPlayback(int32 width, int32 height)
{
	status_t ret = B_OK;

	print_info("init playback with size: %ld x %ld\n", width, height);

	bool fullFrameRate = (width * height) < (800 * 600);
	fullFrameRate = fullFrameRate || fForceFullFrameRate;
	if (fullFrameRate)
		print_info("using full framerate\n");

	fPlaybackManager = new SimplePlaybackManager();
	ret = fPlaybackManager->Init(fVideoView, width, height,
		fDocument, fullFrameRate, fIgnoreNoOverlay);

	if (ret < B_OK) {
		print_error("PlayerWindow::InitPlayback() - "
			"failed to init playback: %s\n", strerror(ret));
		fatal(ret);
	}

	fPlaybackManager->AddListener(fApp);
}

// ShutdownPlayback
void
PlayerWindow::ShutdownPlayback(bool disconnectNodes)
{
	// shutdown playback framework
	if (fPlaybackManager) {
		fPlaybackManager->Shutdown(disconnectNodes);
		delete fPlaybackManager;
		fPlaybackManager = NULL;
	}
}

// #pragma mark -

// StartPlaying
void
PlayerWindow::StartPlaying()
{
	if (fPlaybackManager)
		fPlaybackManager->StartPlaying();
}

// IsPlaying
bool
PlayerWindow::IsPlaying()
{
	return fPlaybackManager && fPlaybackManager->IsPlaying();
}

// GetPlaybackSize
void
PlayerWindow::GetPlaybackSize(int32* width, int32* height) const
{
	if (!fPlaybackManager) {
		*width = DEFAULT_WIDTH;
		*height = DEFAULT_HEIGHT;
		return;
	}
	return fPlaybackManager->GetVideoSize(width, height);
}

// MediaServerLaunched
void
PlayerWindow::MediaServerLaunched()
{
	PostMessage(MSG_MEDIA_SERVER_LAUNCHED);
}

// MediaServerQuit
void
PlayerWindow::MediaServerQuit()
{
	PostMessage(MSG_MEDIA_SERVER_QUIT);
}




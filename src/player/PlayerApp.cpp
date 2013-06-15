/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlayerApp.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <MediaRoster.h>
#include <Roster.h>
#include <Screen.h>

#include "common.h"
#include "svn_revision.h"

#include "AutoDeleter.h"
#include "Clip.h"
#include "ClipObjectFactory.h"
#include "ClipPlaylistItem.h"
#include "DisplaySettings.h"
#include "Document.h"
#include "FileStatusOutput.h"
#include "MessageConstants.h"
#include "MonitorControl.h"
#include "PlayerWindow.h"
#include "PlayerPlaybackNavigator.h"
#include "Playlist.h"
#include "RenderPlaylist.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "ServerObjectManager.h"

using std::nothrow;

static const bigtime_t kMonitorPulseRate = 6000000;

static const char* kMediaServerSig = "application/x-vnd.Be.media-server";
static const char* kMediaServerAddOnSig = "application/x-vnd.Be.addon-host";


// constructor
PlayerApp::PlayerApp()
	: ClockwerkApp(kPlayerMimeSig)
	, fMainWindow(NULL)
	, fTestMode(false)
	, fForceFullFrameRate(false)
	, fPrintPlaylist(false)
	, fIgnoreNoOverlay(false)

	, fControllerMessenger(NULL)

	, fScheduleID("")
	, fSchedule(NULL)
	, fNavigator(new (nothrow) PlayerPlaybackNavigator(fDocument))

	, fDisplayWidth(-1)
	, fDisplayHeight(-1)
	, fDisplayFrequency(-1.0f)
	, fDisplaySource(-1)
	, fTurnOffMonitor(true)
	, fMonitorIsOn(true)
	, fMonitorIsValid(true)
	, fTurnOffDelayStart(LONGLONG_MIN)

	, fUpdateIteration(-1)

	, fLibraryLoaderThread(-1)
	, fLoaderSem(create_sem(0, "loader control sem"))

	, fMonitorControlThread(-1)
	, fMonitorSem(create_sem(0, "monitor control sem"))

	, fMediaServerRunning(false)
	, fMediaAddOnServerRunning(false)
	, fPlaybackWasShutdown(false)

	, fOutput(NULL)
{
	if (!fNavigator) {
		print_error("no memory for playback navigator\n");
		fatal(B_NO_MEMORY);
	}
	PlaybackNavigator::SetDefault(fNavigator);

	if (fLoaderSem >= 0) {
		fLibraryLoaderThread = spawn_thread(_LoaderThreadEntry, "object loader",
			B_NORMAL_PRIORITY, this);
		if (fLibraryLoaderThread >= 0) {
			resume_thread(fLibraryLoaderThread);
		} else {
			print_error("unable to spawn object loader thread: %s\n",
				strerror(fLibraryLoaderThread));
		}
	}
}

// destructor
PlayerApp::~PlayerApp()
{
	// cancel object loader thread
	delete_sem(fLoaderSem);
	if (fLibraryLoaderThread >= 0) {
		int32 exitValue = 0;
		wait_for_thread(fLibraryLoaderThread, &exitValue);
	}

	// cancel monitor controller thread
	delete_sem(fMonitorSem);
	if (fMonitorControlThread >= 0) {
		int32 exitValue = 0;
		wait_for_thread(fMonitorControlThread, &exitValue);
	}

	delete fControllerMessenger;
	SetSchedule(NULL);

	set_global_output(NULL);
	delete fOutput;

	PlaybackNavigator::DeleteDefault();
}

// #pragma mark -

// QuitRequested
bool
PlayerApp::QuitRequested()
{
	fMainWindow->Lock();
	fMainWindow->ShutdownPlayback();
	fMainWindow->Quit();
	fMainWindow = NULL;

	be_roster->StopWatching(BMessenger(this, this));

	return true;
}

// MessageReceived
void
PlayerApp::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_OPEN: {
print_info("received MSG_OPEN command\n");
			if (!fControllerMessenger)
				fControllerMessenger = new BMessenger();
			if (message->FindMessenger("reply target",
					fControllerMessenger) < B_OK
				|| !fControllerMessenger->IsValid()) {
				delete fControllerMessenger;
				fControllerMessenger = NULL;
			}

			if (!fControllerMessenger) {
				print_warning("reply messenger for controller invalid!\n");
			}

			// check the update iteration
			int32 updateIteration;
			if (message->FindInt32("updateIteration", &updateIteration)
					== B_OK) {
				// If it's the same iteration, we don't need to do anything.
				if (updateIteration == fUpdateIteration) {
					print_info("ignoring MSG_OPEN -- old update iteration\n");
					break;
				}
			}

			if (message->FindString("soid", &fScheduleID) == B_OK) {
				// TODO: report error
				// NOTE: the controller currently ignores this message
				if (fControllerMessenger)
					fControllerMessenger->SendMessage(MSG_PLAYLIST_LOADED);
			} else {
				print_error("MSG_OPEN contained no schedule id!\n");
			}

			// see if we should open right now (prevent unnecessary double
			// opening when objects are reloaded anyways, which triggers
			// opening as well)
			bool openNow;
			if (message->FindBool("open now", &openNow) < B_OK)
				openNow = true;
			if (openNow)
				OpenSchedule(fScheduleID);

			break;
		}

		case MSG_UPDATE_OBJECTS:
		{
			BString libraryPath;
			if (message->FindString("library path", &libraryPath) == B_OK) {
				if (libraryPath != fObjectLibrary->Directory()) {
					fObjectLibrary->SetDirectory(libraryPath.String());
				}
			}
			// check the update iteration
			int32 updateIteration;
			if (message->FindInt32("updateIteration", &updateIteration)
					== B_OK) {
				// If it's the same iteration, we don't need to do anything.
				if (updateIteration == fUpdateIteration)
					break;

				fUpdateIteration = updateIteration;
			}

			_TriggerLibraryUpdate();
			break;
		}

		case B_SOME_APP_LAUNCHED:
		case B_SOME_APP_QUIT:
		{
			const char* mimeSig;
			if (message->FindString("be:signature", &mimeSig) < B_OK)
				break;

			bool isMediaServer = strcmp(mimeSig, kMediaServerSig) == 0;
			bool isAddonServer = strcmp(mimeSig, kMediaServerAddOnSig) == 0;
			if (!isMediaServer && !isAddonServer)
				break;

			bool running = (message->what == B_SOME_APP_LAUNCHED);
			if (isMediaServer) {
				fMediaServerRunning = running;
				// if we did not detect the media_server shutting down,
				// fMediaAddOnServerRunning may still be true, even
				// the addon server is not yet running
				fMediaAddOnServerRunning
					= be_roster->IsRunning(kMediaServerAddOnSig);
			}
			if (isAddonServer)
				fMediaAddOnServerRunning = running;

			if (!fMediaServerRunning && !fMediaAddOnServerRunning) {
				print_error("media server has quit.\n");
				// trigger closing of media nodes
				fMainWindow->MediaServerQuit();
				fPlaybackWasShutdown = true;
			} else if (fMediaServerRunning && fMediaAddOnServerRunning) {
				print_error("media server has launched.\n");
				// HACK!
				// quit our now invalid instance of the media roster
				// so that before new nodes are created,
				// we get a new roster (it is a normal looper)
				BMediaRoster* roster = BMediaRoster::CurrentRoster();
				if (roster) {
					roster->Lock();
					roster->Quit();
				}
				if (!fPlaybackWasShutdown) {
					// we may not have detected the media_server quitting
					// earlier (for example when it was killed), make sure
					// the playback is stopped before we re-init it
					fMainWindow->MediaServerQuit();
				}
				// give the servers some time to init...
				snooze(3000000);
				// trigger re-init of media nodes
				fMainWindow->MediaServerLaunched();
				fPlaybackWasShutdown = false;
			}
			break;
		}

		default:
			ClockwerkApp::MessageReceived(message);
			break;
	}
}

// ArgvReceived
void
PlayerApp::ArgvReceived(int32 argCount, char** args)
{
	if (argCount <= 1)
		return;

	args++;
	argCount--;

	while (argCount--) {
		if (!strcmp(args[0], "--test-mode")
				|| !strcmp(args[0], "--test")
				|| !strcmp(args[0], "-t")) {
			fTestMode = true;
		} else if (!strcmp(args[0], "--logging")
				|| !strcmp(args[0], "--log")
				|| !strcmp(args[0], "-l")) {
			if (argCount == 0) {
				printf("please specify the path to the logfile\n");
				exit(0);
			}
			argCount--;
			args++;

			_EnableFileLogging(args[0]);
		} else if (!strcmp(args[0], "--full-frame-rate")
				|| !strcmp(args[0], "-ffr")) {
			fForceFullFrameRate = true;
		} else if (!strcmp(args[0], "--print-playlist")
				|| !strcmp(args[0], "--pp")
				|| !strcmp(args[0], "-p")) {
			fPrintPlaylist = true;
		} else if (!strcmp(args[0], "--allow-bitmap")
				|| !strcmp(args[0], "-b")) {
			fIgnoreNoOverlay = true;
		}

		args++;
	}

}

// ReadyToRun
void
PlayerApp::ReadyToRun()
{
	// try to read log file path from settings file
	BFile logFilePathFile(kPlayerLoggingSettingsPath, B_READ_ONLY);
	if (logFilePathFile.InitCheck() >= B_OK) {
		char buffer[B_PATH_NAME_LENGTH];
		ssize_t read = logFilePathFile.Read(buffer,
			B_PATH_NAME_LENGTH - 1);
		if (read > 0) {
			// terminate the string
			// no guarantee that the path is valid and makes sense, but
			// that's no problem
			buffer[read] = 0;
			BString logFilePath(buffer);
			logFilePath.RemoveAll("\n");
			// enable logging (may be overruled later by command line args)
			_EnableFileLogging(logFilePath.String());
		}
	}
	logFilePathFile.Unset();

	print_info("player started, SVN: %ld\n", kSVNRevision);

	fObjectLibrary = new ServerObjectManager();
	fObjectLibrary->SetLocker(fDocument);

	// the navigator needs the library to find playlists for navigation
	fNavigator->SetObjectManager(fObjectLibrary);

	BRect frame;
	window_feel windowFeel;
	if (modifiers() & B_SHIFT_KEY)
		fTestMode = true;

	if (fTestMode) {
		frame = BRect(50, 50, 50 + 684 - 1, 50 + 384 - 1);
		windowFeel = B_NORMAL_WINDOW_FEEL;
	} else {
		frame = BScreen(B_MAIN_SCREEN_ID).Frame();
		windowFeel = B_FLOATING_ALL_WINDOW_FEEL;
	}

	if (!fTestMode) {
		if (fMonitorIsValid && fMonitorSem >= 0) {
			fMonitorControlThread = spawn_thread(_MonitorControlThreadEntry,
				"monitor controller", B_NORMAL_PRIORITY, this);
			if (fMonitorControlThread >= 0) {
				resume_thread(fMonitorControlThread);
			} else {
				print_error("unable to spawn monitor controller thread: %s\n",
					strerror(fMonitorControlThread));
			}
		}
	}

	// Now tell the application roster, that we're interested
	// in getting notifications of apps being launched or quit.
	// In this way we are going to detect a media_server restart.
	be_roster->StartWatching(BMessenger(this, this),
		B_REQUEST_LAUNCHED | B_REQUEST_QUIT);
	// we will keep track of the status of media_server
	// and media_addon_server
	fMediaServerRunning =  be_roster->IsRunning(kMediaServerSig);
	fMediaAddOnServerRunning = be_roster->IsRunning(kMediaServerAddOnSig);

	// open and start main window
	fMainWindow = new PlayerWindow(frame, windowFeel, this, fDocument,
		fNavigator, fTestMode, fForceFullFrameRate, fIgnoreNoOverlay);

	fMainWindow->StartPlaying();
}

// #pragma mark -

// PlayModeChanged
void
PlayerApp::PlayModeChanged(int32 mode)
{
	// not interested
}

// SwitchPlaylistIfNecessary
void
PlayerApp::SwitchPlaylistIfNecessary()
{
	if (LockWithTimeout(2000) < B_OK)
		return;

	// make sure the playlist is playling at all
	Playlist* masterPlaylist = fNavigator->MasterPlaylist();
	if (fMainWindow->CurrentPlaylist() != masterPlaylist) {
		// set the master playlist
		// make sure that the playback frame of the playlist is
		// at the correct value for the given time of the day
		int64 startFrameOffset = 0;
		if (masterPlaylist->Duration() > 0) {
			// get current daytime
			time_t t = time(NULL);
			tm time = *localtime(&t);

			// calc the current frame of the day since 0:00.00
			int64 frame = (time.tm_hour * 60 * 60 + time.tm_min * 60
				+ time.tm_sec) * 25;

			startFrameOffset = frame % masterPlaylist->Duration();
		}

		fDocument->SetPlaylist(masterPlaylist);
		fMainWindow->SetPlaylist(masterPlaylist, startFrameOffset);
	}

	Unlock();
}

// CurrentFrameChanged
void
PlayerApp::CurrentFrameChanged(double frame)
{
	bool wasPlaying;
	bool isPlaying;
	fNavigator->SetCurrentFrame(frame, wasPlaying, isPlaying);

	if (LockWithTimeout(2000) < B_OK)
		return;

	fTurnOffMonitor = !isPlaying;

	if (wasPlaying != isPlaying && !fTestMode) {
		if (isPlaying) {
			// turn on monitor only if there is anything to play
			// send pulse message, so monitor is turned on as soon
			// as possible, not only at the next regular pulse
			_TriggerMonitorUpdate();
		} else {
			// the monitor will be turned off with a delay (pulse rate),
		 	// in case it is currently on
		 	fTurnOffDelayStart = system_time();
		}
	}

	Unlock();
}

// #pragma mark -

// OpenSchedule
void
PlayerApp::OpenSchedule(const BString& scheduleID)
{
	if (!Lock())
		return;

	if (scheduleID.Length() > 0) {
		AutoReadLocker locker(fDocument);
		// load the new schedule
		Schedule* schedule = dynamic_cast<Schedule*>(
			fObjectLibrary->FindObject(scheduleID));
		if (schedule) {
			print_info("loaded schedule after 'open' command: '%s', "
				"version %ld\n",
				schedule->Name().String(), schedule->Version());
			SetSchedule(schedule);
		}
	}

	if (fPrintPlaylist) {
		Playlist* dummy = _GenerateSchedulePlaylist(fSchedule);
		dummy->PrintToStream();
		delete dummy;
	}

	Unlock();
}

// SetSchedule
void
PlayerApp::SetSchedule(Schedule* schedule)
{
	if (fSchedule == schedule)
		return;

	if (fSchedule)
		fSchedule->Release();

	fSchedule = schedule;

	if (fSchedule) {
		fSchedule->Acquire();
		fSchedule->SanitizeStartFrames();
		fNavigator->SetSchedule(fSchedule);
	}

	BString name = fSchedule ? fSchedule->Name() : BString("<NULL>");
	print_info("switching to schedule '%s'\n", name.String());
}


// #pragma mark -

// _TriggerLibraryUpdate
void
PlayerApp::_TriggerLibraryUpdate()
{
	// there is a great chance that we are supposed to play
	// something, make sure we turn on the monitor ASAP
	fTurnOffMonitor = false;

	// cause loader thread to wakeup immediately
	release_sem(fLoaderSem);
}

// _UpdateLibrary
void
PlayerApp::_UpdateLibrary()
{
	// reinit the ClipLibrary from the ServerObjectManager
	status_t ret = _ReInitClipsFromDisk(false, false);
	if (ret < B_OK) {
		print_error("PlayerApp::_UpdateLibrary(): _ReInitClips() - %s\n",
			   strerror(ret));
		// TODO: think about what should happen...
		// possible errors here: no memory to instantiate objects
		// error resolving dependencies
	}

	// post process objects
	_SyncDisplaySettings();
	_ValidatePlaylistAndScheduleLayouts();

	OpenSchedule(fScheduleID);
}

// _SyncDisplaySettings
void
PlayerApp::_SyncDisplaySettings()
{
return;
	// TODO: untested for deadlocks! now executed in
	// loader thread!

	AutoReadLocker locker(fDocument);
	if (!locker.IsLocked()) {
		print_error("PlayerApp::_SyncDisplaySettings() - unable to "
			"read lock document\n");
		return;
	}

	// search for a "DisplaySettings" object
	int32 count = fObjectLibrary->CountObjects();
	DisplaySettings* settings = NULL;
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = fObjectLibrary->ObjectAtFast(i);
		settings = dynamic_cast<DisplaySettings*>(object);
		if (settings)
			break;
	}
	if (settings && fMainWindow->Lock()) {
		if (!fTestMode) {
			MonitorControl control;

			if (fDisplayWidth != settings->DisplayWidth()
				|| fDisplayHeight != settings->DisplayHeight()
				|| fDisplayFrequency != settings->DisplayFrequency()) {
				status_t status = control.SetMode(*settings);
				if (status < B_OK) {
					print_error("applying display settings to monitor "
						"failed: %s\n", strerror(status));
				}

				// no matter if it failed or not, we don't need to try to
				// set them again...
				fDisplayWidth = settings->DisplayWidth();
				fDisplayHeight = settings->DisplayHeight();
				fDisplayFrequency = settings->DisplayFrequency();
			}

			if (fMonitorIsValid && fDisplaySource != settings->InputSource()) {
				status_t status = control.SetSource(*settings);
				if (status == B_OK)
					fDisplaySource = settings->InputSource();
			}
		}

		// we have a DisplaySettings object, it makes sense
		// to compare the current playback size with the settings
		// and adjust if necessary
		int32 width;
		int32 height;
		fMainWindow->GetPlaybackSize(&width, &height);
		if (width != settings->Width() || height != settings->Height()) {
print_info("switching to video size: %ld, %ld\n",
	   settings->Width(), settings->Height());
			fMainWindow->ShutdownPlayback();
			snooze(100000);
			fMainWindow->InitPlayback(settings->Width(), settings->Height());
		}
		fMainWindow->Unlock();
	}
}

// _ValidatePlaylistAndScheduleLayouts
void
PlayerApp::_ValidatePlaylistAndScheduleLayouts()
{
	AutoWriteLocker locker(fDocument);
	if (!locker.IsLocked()) {
		print_error("PlayerApp::_ValidatePlaylistAndScheduleLayouts() - unable to "
			"write lock document\n");
		return;
	}

	_ValidatePlaylistLayouts();

	if (fSchedule)
		fSchedule->SanitizeStartFrames();
}

// #pragma mark -

// _GenerateSchedulePlaylist
Playlist*
PlayerApp::_GenerateSchedulePlaylist(const Schedule* schedule) const
{
	if (!schedule)
		return NULL;

	Playlist* playlist = new (nothrow) Playlist();
	if (!playlist)
		return NULL;

	playlist->SetName("Schedule converted Playlist");

	ObjectDeleter<Playlist> playlistDeleter(playlist);

	int32 count = schedule->CountItems();
	for (int32 i = 0; i < count; i++) {
		ScheduleItem* scheduleItem = schedule->ItemAtFast(i);
		Playlist* itemPlaylist = scheduleItem->Playlist();
		uint64 startFrame = scheduleItem->StartFrame();
		int64 itemDuration = scheduleItem->Duration();
		if (startFrame > (uint64)kWholeDayDuration
			|| itemDuration > kWholeDayDuration) {
			// ignore invalid items
			continue;
		}
		if (itemPlaylist) {
			// repeat adding playlist items for as long as the playlist
			// is repeating in this schedule item
			while (itemDuration > 0) {
				ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(
					itemPlaylist);
				if (!item)
					return NULL;

				uint64 duration = min_c((uint64)itemDuration,
					itemPlaylist->Duration());
				if (duration == 0) {
					print_warning("skipping empty playlist\n");
					delete item;
					break;
				}

				item->SetStartFrame(startFrame);
				item->SetDuration(duration);
				if (!playlist->AddItem(item)) {
					delete item;
					return NULL;
				}
				itemDuration -= duration;
				startFrame += duration;
			}
		} else {
			ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem(
				(Clip*)NULL);
			if (!item)
				return NULL;

			item->SetStartFrame(startFrame);
			item->SetDuration(itemDuration);

			if (!playlist->AddItem(item)) {
				delete item;
				return NULL;
			}
		}
	}

	playlistDeleter.Detach();
	return playlist;
}

// #pragma mark -

// _EnableFileLogging
void
PlayerApp::_EnableFileLogging(const char* pathToLogFile)
{
	fOutput = new FileStatusOutput(pathToLogFile);
	if (fOutput->InitCheck() < B_OK) {
		printf("error creating log file at '%s'\n", pathToLogFile);
		delete fOutput;
		fOutput = NULL;
		return;
	}
	set_global_output(fOutput);
}

// #pragma mark -

// _LoaderThreadEntry
int32
PlayerApp::_LoaderThreadEntry(void* cookie)
{
	PlayerApp* app = (PlayerApp*)cookie;
	return app->_LoaderThread();
}

// _LoaderThread
int32
PlayerApp::_LoaderThread()
{
	while (true) {
		// wait a bit
		status_t ret = acquire_sem_etc(fLoaderSem, 1, B_RELATIVE_TIMEOUT,
			60 * 1000000);
		if (ret == B_OK) {
			// B_OK means we are supposed to do something
			_UpdateLibrary();
		} else if (ret != B_TIMED_OUT && ret != B_INTERRUPTED) {
			// we are supposed to quit
			break;
		}
	}
	return 0;
}

// #pragma mark -

// _MonitorControlThreadEntry
int32
PlayerApp::_MonitorControlThreadEntry(void* cookie)
{
	PlayerApp* app = (PlayerApp*)cookie;
	return app->_MonitorControlThread();
}

// _MonitorControlThread
int32
PlayerApp::_MonitorControlThread()
{
	MonitorControl control;
	fMonitorIsValid = control.Init() == B_OK;
	if (fMonitorIsValid) {
		fMonitorIsOn = control.On();
	} else {
		print_error("unable to initialize serial port, "
			"monitor control is disabled\n");
	}

	while (true) {
		// wait a bit
		status_t ret = acquire_sem_etc(fMonitorSem, 1, B_RELATIVE_TIMEOUT,
			kMonitorPulseRate);
		if (ret == B_OK) {
			// B_OK means we are supposed to do something
			_ControlMonitor();
		} else if (ret != B_TIMED_OUT && ret != B_INTERRUPTED) {
			// we are supposed to quit
			break;
		}
	}
	return 0;
}

// _TriggerMonitorUpdate
void
PlayerApp::_TriggerMonitorUpdate()
{
	// cause monitor control thread to wakeup immediately
	release_sem(fMonitorSem);
}

// _ControlMonitor
void
PlayerApp::_ControlMonitor()
{
	if (fTestMode || !fMonitorIsValid)
		return;

	bool turnOff = false;
	bigtime_t turnOffDelayStart = LONGLONG_MIN;

	if (Lock()) {
		turnOff = fTurnOffMonitor;
		turnOffDelayStart = fTurnOffDelayStart;
		Unlock();
	}

	if (turnOff && fMonitorIsOn
		&& turnOffDelayStart + kMonitorPulseRate < system_time()) {
		print_info("monitor needs to be turned OFF\n");
		MonitorControl control;
		fMonitorIsOn = control.TurnOff() != B_OK;
	} else if (!turnOff && !fMonitorIsOn) {
		print_info("monitor needs to be turned ON\n");
		_TurnOnMonitor();
	}
}

// _TurnOnMonitor
void
PlayerApp::_TurnOnMonitor()
{
	MonitorControl control;
	fMonitorIsOn = control.TurnOn() == B_OK;
	if (!fMonitorIsOn) {
		print_error("failed to turn monitor on (it might still be on)\n");
		return;
	}

	snooze(100000);

	control.EnableRemote(false);

	snooze(100000);

	control.AutoAdjust();

	print_info("monitor turned on successfully\n");
}



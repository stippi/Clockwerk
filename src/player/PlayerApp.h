/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYER_APP_H
#define PLAYER_APP_H

#include <String.h>

#include "ClockwerkApp.h"
#include "PlaybackListener.h"

class FileStatusOutput;
class PlayerWindow;
class PlayerPlaybackNavigator;
class Playlist;
class Schedule;

class PlayerApp : public ClockwerkApp, public PlaybackListener {
 public:
								PlayerApp();
	virtual						~PlayerApp();

	// BApplication interface
	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);
	virtual void				ArgvReceived(int32 argCount, char** args);
	virtual	void				ReadyToRun();

	// PlaybackListener interface
	virtual	void				PlayModeChanged(int32 mode);
	virtual	void				CurrentFrameChanged(double frame);
	virtual	void				SwitchPlaylistIfNecessary();

	// PlayerApp
			void				OpenSchedule(const BString& scheduleID);
			void				SetSchedule(Schedule* schedule);

 private:
			void				_TriggerLibraryUpdate();
			void				_UpdateLibrary();

			void				_SyncDisplaySettings();
			void				_ValidatePlaylistAndScheduleLayouts();

			Playlist*			_GenerateSchedulePlaylist(
									const Schedule* schedule) const;

			void				_EnableFileLogging(const char* pathToLogFile);

			void				_TriggerMonitorUpdate();
			void				_ControlMonitor();
			void				_TurnOnMonitor();

	static	int32				_LoaderThreadEntry(void* cookie);
			int32				_LoaderThread();

	static	int32				_MonitorControlThreadEntry(void* cookie);
			int32				_MonitorControlThread();

			PlayerWindow*		fMainWindow;
			bool				fTestMode;
			bool				fForceFullFrameRate;
			bool				fPrintPlaylist;
			bool				fIgnoreNoOverlay;

			BMessenger*			fControllerMessenger;

			BString				fScheduleID;
			Schedule*			fSchedule;
			PlayerPlaybackNavigator* fNavigator;

			// last set display settings
			int32				fDisplayWidth;
			int32				fDisplayHeight;
			float				fDisplayFrequency;
			int32				fDisplaySource;
	volatile bool				fTurnOffMonitor;
			bool				fMonitorIsOn;
			bool				fMonitorIsValid;
			bigtime_t			fTurnOffDelayStart;

			int32				fUpdateIteration;

			thread_id			fLibraryLoaderThread;
			sem_id				fLoaderSem;

			thread_id			fMonitorControlThread;
			sem_id				fMonitorSem;

			bool				fMediaServerRunning;
			bool				fMediaAddOnServerRunning;
			bool				fPlaybackWasShutdown;

			FileStatusOutput*	fOutput;
};

#endif // PLAYER_APP_H

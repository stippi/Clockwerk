/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SIMPLE_PLAYBACK_MANAGER_H
#define SIMPLE_PLAYBACK_MANAGER_H

#include <OS.h>
#include <List.h>
#include <Locker.h>
#include <MediaDefs.h>
#include <MediaNode.h>

#include "ClipRendererCache.h"
#include "PlaybackManagerInterface.h"
#include "Painter.h"

class AudioProducer;
class BBitmap;
class BView;
class Playlist;
class PlaylistAudioSupplier;
class PlaybackListener;
class RWLocker;
class TimeSource;

#define MAX_BUFFER_COUNT	64
#define DEFAULT_WIDTH		684
#define DEFAULT_HEIGHT		384

// TODO: when playback is supposed to stop after the last frame
// of a playlist, only the rendering will stop then, the display
// thread will stop too early and the last remaining frames are
// never displayed

class SimplePlaybackManager : public PlaybackManagerInterface {
 public:
								SimplePlaybackManager();
	virtual						~SimplePlaybackManager();

	// PlaybackManagerInterface
	virtual	bool				Lock();
	virtual	status_t			LockWithTimeout(bigtime_t timeout);
	virtual	void				Unlock();

	virtual	void				GetPlaylistTimeInterval(
									bigtime_t startTime, bigtime_t& endTime,
									bigtime_t& contentStartTime,
									bigtime_t& contentEndTime,
									float& playingSpeed) const;

	virtual	void				SetCurrentAudioTime(bigtime_t time);

	// SimplePlaybackManager
			status_t			Init(BView* displayTarget,
									int32 width, int32 height,
									RWLocker* locker,
									bool fullFrameRate = false,
									bool ignoreNoOverlay = false);
			void				Shutdown(bool disconnectNodes = true);

			void				SetPlaylist(::Playlist* playlist,
									int64 startFrameOffset);
			::Playlist*			Playlist() const
									{ return fPlaylist; }

			void				StartPlaying();
			void				StopPlaying();
			bool				IsPlaying() const;

			float				VideoFramesPerSecond() const;
			void				GetVideoSize(int32* width,
									int32* height) const;

			void				AddListener(PlaybackListener* listener);
			void				RemoveListener(PlaybackListener* listener);

 private:
			struct Connection {
					Connection();
		
					media_node			producer;
					media_node			consumer;
					media_source		source;
					media_destination	destination;
					media_format		format;
					bool				connected;
			};

			status_t			_InitAudio();
			status_t			_ShutdownAudio(bool disconnect = true);
			status_t			_StartAudio();
			void				_StopAudio();

	static	int32				_FrameGeneratorEntry(void* cookie);
			void				_FrameGenerator();
	static	int32				_FrameDisplayerEntry(void* cookie);
			void				_FrameDisplayer();

			bool				_SetOverlay(BBitmap* bitmap);

			// trigger PlaybackListener notifications:
			void				_PlaybackStarted();
			void				_PlaybackStopped();
			void				_CurrentFrameChanged(double currentFrame);
			void				_SwitchPlaylistIfNecessary();

			void				_PrintAvailableOverlayColorspaces(BRect bounds);

			::Playlist*			fPlaylist;
			Painter				fPainter;
			ClipRendererCache	fRendererCache;
			AudioProducer*		fAudioProducer;
			Connection			fAudioConnection;
			PlaylistAudioSupplier* fAudioSupplier;
			media_node			fTimeSourceNode;
			RWLocker*			fLocker;
			BView*				fVideoView;
			int32				fFrameRateScale;
			int32				fWidth;
			int32				fHeight;
	volatile double				fCurrentFrame;
			BLocker				fLock;

	volatile int64				fLastDisplayedFrame;
	volatile bigtime_t			fLastDisplayedFrameRealtime;

			BBitmap*			fOverlayBitmap[MAX_BUFFER_COUNT];
			BLocker				fBufferLock[MAX_BUFFER_COUNT];
			int64				fPlaybackFrame[MAX_BUFFER_COUNT];
			uint32				fBufferCount;

			thread_id			fGeneratorThread;
			thread_id			fDisplayerThread;
	volatile bool				fQuitting;
	volatile bool				fPaused;
	volatile bool				fHurryUp;

			bool				fClearViewWithOverlayColor;

			bigtime_t			fPerformanceStartTime;
			bigtime_t			fRealStartTime;
	volatile int64				fFrameCountSinceStart;
	volatile int64				fPlaylistStartFrameOffset;
	volatile int64				fLastPlaylistSwitchFrame;

	// BTimeSource info
			TimeSource*			fTimeSource;
			BTimeSource*		fAudioTimeSource;

	// performance measuring
			bigtime_t			fGenerateTime;
			bigtime_t			fCopyTime;
			bigtime_t			fBlankTime;
			bigtime_t			fDisplayTime;
			uint64				fFrameCount;

	// listeners
			BList				fListeners;
			BLocker				fListenersLock;
};

#endif // SIMPLE_PLAYBACK_MANAGER_H


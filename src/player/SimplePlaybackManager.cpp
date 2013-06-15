/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SimplePlaybackManager.h"

#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <MediaRoster.h>
#include <View.h>
#include <Region.h>
#include <scheduler.h>
#include <Screen.h>

#include "common.h"

#include "AudioProducer.h"
#include "AutoLocker.h"
#include "BBitmapBuffer.h"
#include "Debug.h"
#include "Painter.h"
#include "PlaybackManager.h"
#include "PlaybackListener.h"
#include "Playlist.h"
#include "PlaylistAudioSupplier.h"
#include "RenderPlaylist.h"
#include "RWLocker.h"
#include "TimeSource.h"

// debugging
#include "Debug.h"
//#define ldebug	debug
#define ldebug	nodebug


SimplePlaybackManager::Connection::Connection()
	: connected(false)
{
	memset(&format, 0, sizeof(media_format));
}


// clear_bitmap
void
clear_bitmap(BBitmap* bitmap, BRect area)
{
	// clip area to bitmap bounds
	BRect bitmapBounds = bitmap->Bounds();
	area = area & bitmapBounds;
	// handle bitmap bounds not located at B_ORIGIN
	area.OffsetBy(-bitmapBounds.left, -bitmapBounds.top);

	uint32 width = area.IntegerWidth() + 1;
	uint32 height = area.IntegerHeight() + 1;

	bitmap->LockBits();

	uint8* bits = (uint8*)bitmap->Bits();
	uint32 bpr = bitmap->BytesPerRow();

	if (bitmap->ColorSpace() == B_YCbCr422) {
		// offset bits to left top pixel in area
		bits += (int32)area.top * bpr + (int32)area.left * 2;
	
		while (height--) {
			uint8* handle = bits;
		    for (uint32 x = 0; x < width; x++) {
				handle[0] = 16;
				handle[1] = 128;
				handle += 2;
			}
			// next row
			bits += bpr;
		}
	} else if (bitmap->ColorSpace() == B_YCbCr444) {
		// offset bits to left top pixel in area
		bits += (int32)area.top * bpr + (int32)area.left * 3;
	
		while (height--) {
			uint8* handle = bits;
		    for (uint32 x = 0; x < width; x++) {
				handle[0] = 16;
				handle[1] = 128;
				handle[2] = 128;
				handle += 3;
			}
			// next row
			bits += bpr;
		}
	} else {
		memset(bitmap->Bits(), 0, bitmap->BitsLength());
	}

	bitmap->UnlockBits();
}

// #pragma mark -

// constructor
SimplePlaybackManager::SimplePlaybackManager()
	: fPlaylist(NULL),
	  fPainter(),
	  fAudioProducer(NULL),
	  fAudioSupplier(NULL),
	  
	  fLocker(NULL),
	  fVideoView(NULL),
	  fFrameRateScale(2),
	  fWidth(DEFAULT_WIDTH),
	  fHeight(DEFAULT_HEIGHT),
	  fCurrentFrame(0.0),

	  fLastDisplayedFrame(-1),
	  fLastDisplayedFrameRealtime(0),

	  fBufferCount(0),

	  fGeneratorThread(-1),
	  fDisplayerThread(-1),
	  fQuitting(false),
	  fPaused(true),
	  fHurryUp(false),

	  fClearViewWithOverlayColor(true),

	  fPerformanceStartTime(0),
	  fFrameCountSinceStart(0),
	  fPlaylistStartFrameOffset(0),
	  fLastPlaylistSwitchFrame(0),

	  fTimeSource(new (std::nothrow) TimeSource()),
	  fAudioTimeSource(fTimeSource),

	  fGenerateTime(0),
	  fCopyTime(0),
	  fBlankTime(0),
	  fDisplayTime(0),
	  fFrameCount(0),

	  fListeners(2)
{
	for (int32 i = 0; i < MAX_BUFFER_COUNT; i++) {
		fOverlayBitmap[i] = NULL;
		fPlaybackFrame[i] = 0;
	}
}

// destructor
SimplePlaybackManager::~SimplePlaybackManager()
{
	SetPlaylist(NULL, 0);

	// print performance
	if (fFrameCount == 0)
		return;

	print_info("average time to generate: %lld\n", fGenerateTime / fFrameCount);
	print_info("     scratch buffer copy: %lld\n", fCopyTime / fFrameCount);
	print_info("            clear buffer: %lld\n", fBlankTime / fFrameCount);
	print_info("                 display: %lld\n", fDisplayTime / fFrameCount);
	print_info("              video size: %ld x %ld\n", fWidth, fHeight);

	if (fTimeSource)
		fTimeSource->Release();
}

// #pragma mark -

// Lock
bool
SimplePlaybackManager::Lock()
{
	return fLock.Lock();
}

// LockWithTimeout
status_t
SimplePlaybackManager::LockWithTimeout(bigtime_t timeout)
{
	return fLock.LockWithTimeout(timeout);
}

// Unlock
void
SimplePlaybackManager::Unlock()
{
	fLock.Unlock();
}

// GetPlaylistTimeInterval
void
SimplePlaybackManager::GetPlaylistTimeInterval(
	bigtime_t performanceStartTime, bigtime_t& performanceEndTime,
	bigtime_t& contentStartTime, bigtime_t& contentEndTime,
	float& playingSpeed) const
{
	// NOTE: this function is executed in the audio playback thread,
	// but fLock has been locked, this is needed since the fPlaylist
	// pointer may be modified from the frame generator thread
	playingSpeed = 1.0;
	if (!fPlaylist)
		return;

	int64 frameCount = fPlaylist->Duration();
	bigtime_t duration = (bigtime_t)(frameCount * 1000000.0
		/ fPlaylist->VideoFrameRate());
	if (duration == 0)
		return;

	// map the performance times by the playlist start frame offset
	bigtime_t offset = (bigtime_t)((fPlaylistStartFrameOffset
		- fLastPlaylistSwitchFrame) * 1000000.0 / fPlaylist->VideoFrameRate());
	performanceStartTime += offset;
	bigtime_t offsetPerformanceEndTime = performanceEndTime + offset;

	contentStartTime = performanceStartTime % duration;
	contentEndTime = contentStartTime + (offsetPerformanceEndTime
		- performanceStartTime);
	if (contentEndTime > duration) {
		bigtime_t diff = contentEndTime - duration;
		contentEndTime -= diff;
		performanceEndTime -= diff;
	}
//
//printf("duration: %lld, interval: %lld -> %lld\n",
//	duration, contentStartTime, contentEndTime);
}

// SetCurrentAudioTime
void
SimplePlaybackManager::SetCurrentAudioTime(bigtime_t time)
{
}

// #pragma mark -

// Init
status_t
SimplePlaybackManager::Init(BView* displayTarget, int32 width, int32 height,
	RWLocker* locker, bool fullFrameRate, bool ignoreNoOverlay)
{
	AutoLocker<BLocker> _(fLock);

	// clear any old stuff
	Shutdown();

	fLocker = locker;
	fVideoView = displayTarget;
	fWidth = width;
	fHeight = height;

	BRect bounds(0, 0, fWidth - 1, fHeight - 1);

	uint32 bitmapFlags = B_BITMAP_WILL_OVERLAY;
	color_space bitmapFormat = B_YCbCr422;

	status_t ret = B_OK;

	int32 maxTries = ignoreNoOverlay ? 1 : 10;
	int32 tries = 1;
	while (tries <= maxTries) {
		fOverlayBitmap[0] = new BBitmap(bounds,
			bitmapFlags | B_BITMAP_RESERVE_OVERLAY_CHANNEL, bitmapFormat);
	
		ret = fOverlayBitmap[0]->InitCheck();
		if (ret == B_OK)
			break;

		print_warning("unable to obtain overlay, attempt %ld of %ld\n",
			tries, maxTries);
		delete fOverlayBitmap[0];
		snooze(100000 * tries);
		tries++;
	}

	if (ret < B_OK) {
		print_error("failed creating overlay 1 (YCbCr422, %ldx%ld): %s\n",
			bounds.IntegerWidth() + 1, bounds.IntegerHeight() + 1,
			strerror(ret));

		_PrintAvailableOverlayColorspaces(bounds);
		// actually this is a bad problem (no overlays), but we can
		// carry on, so that we see something on screen and are able
		// to test the system (pass -b option on command line)
		if (!ignoreNoOverlay)
			return ret;

		// continue with normal bitmaps
		bitmapFlags = 0;
		bitmapFormat = B_RGB32;

		fOverlayBitmap[0] = new BBitmap(bounds, bitmapFlags, bitmapFormat);
		ret = fOverlayBitmap[0]->InitCheck();
	}

	fBufferCount = 1;

	for (int32 i = 1; i < MAX_BUFFER_COUNT; i++) {
		fOverlayBitmap[i] = new BBitmap(bounds,
										bitmapFlags,
										bitmapFormat);
	
		ret = fOverlayBitmap[i]->InitCheck();
		if (ret < B_OK) {
			delete fOverlayBitmap[i];
			fOverlayBitmap[i] = NULL;
			if (fBufferCount < 4) {
				print_error("failed creating overlay %ld "
					   "(need at least 3): %s\n", i + 1, strerror(ret));
				// we want at least 3 buffers
				return ret;
			} else {
				ret = B_OK;
				break;
			}
		} else {
			clear_bitmap(fOverlayBitmap[i], bounds);
			fBufferCount++;
		}
	}
//printf("managed to allocate %ld overlays\n", fBufferCount);

	BBitmapBuffer buffer(fOverlayBitmap[0]);
	if (!fPainter.AttachToBuffer(&buffer)) {
		ret = B_UNSUPPORTED;
		return ret;
	}

	clear_bitmap(fOverlayBitmap[0], bounds);

	if (bitmapFormat == B_YCbCr422) {
		// set initial overlay, mostly to get the overlay color key
		// the call can fail, but we try several times
		rgb_color key;
		int32 tries = 20;
		do {
			if (tries < 20)
				snooze(20000);
			ret = fVideoView->SetViewOverlay(fOverlayBitmap[0], bounds,
					   fVideoView->Bounds(), &key, B_FOLLOW_ALL,
					   B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
			tries --;
		} while (ret < B_OK && tries > 0);
		if (ret < B_OK)
			return ret;

		fVideoView->SetViewColor(key);
	} else {
		fVideoView->SetViewColor(0, 0, 0, 255);
	}

	fQuitting = false;
	fFrameRateScale = fullFrameRate ? 1 : 2;

	// init time source
	if (fTimeSource)
		ret = fTimeSource->Init();
	else
		ret = B_NO_MEMORY;

	if (ret < B_OK)
		return ret;

	// init audio producer
	ret = _InitAudio();
	if (ret < B_OK)
		return ret;

	// span frame generator thread
	fGeneratorThread = spawn_thread(_FrameGeneratorEntry, "frame generator",
									B_NORMAL_PRIORITY, this);
	if (fGeneratorThread >= B_OK)
		ret = resume_thread(fGeneratorThread);
	else
		ret = fGeneratorThread;

	if (ret < B_OK)
		return ret;

	// span frame displayer thread
	fDisplayerThread = spawn_thread(_FrameDisplayerEntry, "frame displayer",
									B_REAL_TIME_DISPLAY_PRIORITY, this);
	if (fDisplayerThread >= B_OK)
		ret = resume_thread(fDisplayerThread);
	else
		ret = fDisplayerThread;

	if (ret < B_OK)
		return ret;

	return ret;
}

// Shutdown
void
SimplePlaybackManager::Shutdown(bool disconnectNodes)
{
	_ShutdownAudio(disconnectNodes);

	fQuitting = true;
	if (fDisplayerThread >= 0) {
		status_t ret;
		wait_for_thread(fDisplayerThread, &ret);
	}
	if (fGeneratorThread >= 0) {
		status_t ret;
		wait_for_thread(fGeneratorThread, &ret);
	}

	if (fVideoView) {
		fVideoView->SetViewColor(0, 0, 0, 255);
		fVideoView->SetHighColor(fVideoView->ViewColor());
		fVideoView->FillRect(fVideoView->Bounds());
		fVideoView->ClearViewOverlay();
		fVideoView->Sync();
	}

	for (int32 i = 0; i < MAX_BUFFER_COUNT; i++) {
		snooze(10000);
		delete fOverlayBitmap[i];
		fOverlayBitmap[i] = NULL;
	}
}

// SetPlaylist
void
SimplePlaybackManager::SetPlaylist(::Playlist* playlist, int64 startFrameOffset)
{
	AutoLocker<BLocker> locker(fLock);

	if (fPaused || fPlaylist == playlist)
		return;

	fTimeSource->Lock();

	if (fRealStartTime == 0) {
		fTimeSource->Unlock();
		return;
	}

	if (fLastDisplayedFrameRealtime == 0)
		fLastDisplayedFrameRealtime = fRealStartTime;

	bigtime_t estimatedDisplayRealTime
		= (bigtime_t)(fLastDisplayedFrameRealtime + (fFrameCountSinceStart
			- fLastDisplayedFrame)
			* fTimeSource->TimePerVideoFrame(fFrameRateScale));

	fTimeSource->Unlock();

print_info("switching to playlist: %s, skip to: %lld, "
	"old playlist: %s\n", playlist ?
	playlist->Name().String() : NULL, startFrameOffset,
	fPlaylist ? fPlaylist->Name().String() : NULL);

	if (fPlaylist)
		fPlaylist->Release();

	fPlaylist = playlist;

	// remember the intended playlist start frame, the playback frame and audio portion
	// being played back will be wrapped around this frame
	fPlaylistStartFrameOffset = startFrameOffset;

	bigtime_t estimatedPerformanceTime
		= fAudioTimeSource->PerformanceTimeFor(estimatedDisplayRealTime)
			- fPerformanceStartTime;
	double frameSincePlaybackStart
		= estimatedPerformanceTime * VideoFramesPerSecond() / 1000000;

	fLastPlaylistSwitchFrame = (int64)frameSincePlaybackStart;
//print_info("playback switch at internal frame: %lld\n", fLastPlaylistSwitchFrame);

	if (fPlaylist) {
		fPlaylist->Acquire();

		AffineTransform transform;
		transform.ScaleBy(B_ORIGIN,
						  (fPainter.Bounds().Width() + 1) / fPlaylist->Width(),
						  (fPainter.Bounds().Height() + 1) / fPlaylist->Height());
		fPainter.SetTransformation(transform);
	}

	if (fAudioSupplier)
		fAudioSupplier->SetPlaylist(fPlaylist);
}

// #pragma mark -

// StartPlaying
void
SimplePlaybackManager::StartPlaying()
{
//printf("SimplePlaybackManager::StartPlaying()\n");

// TODO: remove this code? I think what it was supposed to do is now
// done in _StartAudio() (delay the playback start by 30 frames)
//	fTimeSource->Lock();
//
//	fPerformanceStartTime = fTimeSource->lastRetracePerformanceTime
//		+ 30 * fTimeSource->estimatedRetraceDuration;
//	fFrameCountSinceStart = 0;
//	fPaused = false;
//
//	fTimeSource->Unlock();

	// start AudioProducer here (at fPerformanceStartTime)
	_StartAudio();

	_PlaybackStarted();
}

// StopPlaying
void
SimplePlaybackManager::StopPlaying()
{
//printf("SimplePlaybackManager::StopPlaying()\n");
	_StopAudio();

	fPaused = true;
	_PlaybackStopped();
}

// IsPlaying
bool
SimplePlaybackManager::IsPlaying() const
{
	return fPaused == false;
}

// VideoFramesPerSecond
float
SimplePlaybackManager::VideoFramesPerSecond() const
{
	return (fPlaylist ? fPlaylist->VideoFrameRate() : 25.0);
}

// GetVideoSize
void
SimplePlaybackManager::GetVideoSize(int32* width, int32* height) const
{
	if (width)
		*width = fWidth;
	if (height)
		*height = fHeight;
}

// #pragma mark -

// AddListener
void
SimplePlaybackManager::AddListener(PlaybackListener* listener)
{
	if (!fListenersLock.Lock())
		return;

	fListeners.AddItem((void*)listener);

	fListenersLock.Unlock();
}

// RemoveListener
void
SimplePlaybackManager::RemoveListener(PlaybackListener* listener)
{
	if (!fListenersLock.Lock())
		return;

	fListeners.RemoveItem((void*)listener);

	fListenersLock.Unlock();
}


// #pragma mark -

static void
print_media_error(const char* message, status_t error)
{
	print_error("%s: %s\n", message, strerror(error));
}

// _InitAudio
status_t
SimplePlaybackManager::_InitAudio()
{
	status_t ret = B_OK;
	BMediaRoster* roster = BMediaRoster::Roster(&ret);
	if (ret < B_OK)
		return ret;

	AutoLocker<BMediaRoster> locker(roster);
	if (!locker.IsLocked())
		return B_ERROR;

	float videoFPS = VideoFramesPerSecond();
	fAudioSupplier = new PlaylistAudioSupplier(fPlaylist, fLocker, this,
		videoFPS);
	fAudioProducer = new AudioProducer("Clockwerk Audio Out", fAudioSupplier,
		false);

	ret = roster->RegisterNode(fAudioProducer);
	if (ret != B_OK) {
		print_media_error("unable to register audio producer node", ret);
		return ret;
	}
	// make sure the Media Roster knows that we're using the node
//	roster->GetNodeFor(fAudioProducer->Node().node, &fAudioConnection.producer);
	fAudioConnection.producer = fAudioProducer->Node();

	// connect to the mixer
	ret = roster->GetAudioMixer(&fAudioConnection.consumer);
	if (ret != B_OK) {
		print_media_error("unable to get the system mixer", ret);
		return ret;
	}

//	fTimeSourceNode = fTimeSource->Node();

	// find the time source
	ret = roster->GetTimeSource(&fTimeSourceNode);
	if (ret != B_OK) {
		print_media_error("Can't get a time source", ret);
		return ret;
	}

	roster->SetTimeSourceFor(fAudioConnection.producer.node,
							 fTimeSourceNode.node);

	// got the nodes; now we find the endpoints of the connection
	media_input mixerInput;
	media_output soundOutput;
	int32 count = 1;
	ret = roster->GetFreeOutputsFor(fAudioConnection.producer, &soundOutput, 1, &count);
	if (ret != B_OK) {
		print_media_error("unable to get a free output from the producer node", ret);
		return ret;
	}
	count = 1;
	ret = roster->GetFreeInputsFor(fAudioConnection.consumer, &mixerInput, 1, &count);
	if (ret != B_OK) {
		print_media_error("unable to get a free input to the mixer", ret);
		return ret;
	}

	// got the endpoints; now we connect it!
	fAudioConnection.format.type = B_MEDIA_RAW_AUDIO;	
	fAudioConnection.format.u.raw_audio = media_raw_audio_format::wildcard;
	ret = roster->Connect(soundOutput.source, mixerInput.destination,
						  &fAudioConnection.format, &soundOutput, &mixerInput);

//char formatString[1024];
//if (string_for_format(fAudioConnection.format, formatString, 1024))
//printf("connected format: %s\n", formatString);

	if (ret != B_OK) {
		print_media_error("unable to connect audio nodes", ret);
		return ret;
	}

	// the inputs and outputs might have been reassigned during the
	// nodes' negotiation of the Connect().  That's why we wait until
	// after Connect() finishes to save their contents.
	fAudioConnection.source = soundOutput.source;
	fAudioConnection.destination = mixerInput.destination;
	fAudioConnection.connected = true;

	// Set an appropriate run mode for the producer
	ret = roster->SetRunModeNode(fAudioConnection.producer,
								 BMediaNode::B_INCREASE_LATENCY);
	if (ret != B_OK) {
		print_media_error("unable to set run mode", ret);
		return ret;
	}

	return B_OK;
}

// _ShutdownAudio
status_t
SimplePlaybackManager::_ShutdownAudio(bool disconnect)
{
	status_t ret = B_OK;
	BMediaRoster* roster = BMediaRoster::Roster(&ret);
	if (ret != B_OK) {
		if (fAudioProducer) {
			fAudioProducer->Release();
			fAudioProducer = NULL;
		}
		delete fAudioSupplier;
		fAudioSupplier = NULL;
		return ret;
	}

	AutoLocker<BMediaRoster> locker(roster);
	if (!locker.IsLocked())
		return B_ERROR;

	if (fAudioProducer) {
		disconnect = disconnect && fAudioConnection.connected;
		// Ordinarily we'd stop *all* of the nodes in the chain at this point.
		// However, one of the nodes is the System Mixer, and stopping the
		// Mixer is a Bad Idea (tm). So, we just disconnect from it, and
		// release our references to the nodes that we're using.  We *are*
		// supposed to do that even for global nodes like the Mixer.
		if (disconnect) {
ldebug("  disconnecting audio...\n");
			ret = roster->Disconnect(fAudioConnection.producer.node,
									 fAudioConnection.source,
									 fAudioConnection.consumer.node,
									 fAudioConnection.destination);
			if (ret < B_OK) {
				print_media_error("unable to disconnect audio nodes", ret);
				disconnect = false;
			}
		} else {
			print_error("SimplePlaybackManager::_ShutdownAudio() - "
				   "cannot disconnect audio nodes, no media server!\n");
		}

ldebug("  releasing audio producer...\n");
		fAudioProducer->Release();
		fAudioProducer = NULL;
		fAudioConnection.connected = false;

		if (disconnect) {
ldebug("  releasing audio mixer...\n");
			roster->ReleaseNode(fAudioConnection.consumer);
		} else {
			print_error("SimplePlaybackManager::_ShutdownAudio() - "
				   "cannot release audio mixer!\n");
		}
		snooze(20000LL);
	}

	// delete audio supplier
	delete fAudioSupplier;
	fAudioSupplier = NULL;

ldebug("SimplePlaybackManager::_ShutdownAudio() - done\n");
	return ret;
}

// _StartAudio
status_t
SimplePlaybackManager::_StartAudio()
{
	status_t ret = B_NO_INIT;
	if (!fAudioProducer)
		return ret;

	ret = B_OK;
	BMediaRoster* roster = BMediaRoster::Roster(&ret);
	if (ret < B_OK)
		return ret;

	// begin mucking with the media roster
	AutoLocker<BMediaRoster> locker(roster);
	if (!locker.IsLocked())
		return B_ERROR;

	// start the nodes
	fAudioTimeSource = roster->MakeTimeSourceFor(fAudioProducer->Node());

	fTimeSource->Lock();

	fRealStartTime = 
		bigtime_t(fTimeSource->lastRetraceRealTime
			+ 120.0 * fTimeSource->TimePerVideoFrame(fFrameRateScale));

	fPerformanceStartTime = fAudioTimeSource->PerformanceTimeFor(fRealStartTime);

	fFrameCountSinceStart = 0;
	fPaused = false;

	fTimeSource->Unlock();
	
	// start the node
	fAudioProducer->SetRunning(true);
	ret = roster->StartNode(fAudioConnection.producer, fPerformanceStartTime);
	if (ret != B_OK) {
		print_media_error("can't start the audio producer", ret);
		return ret;
	}

	return ret;
}

// _StopAudio
void
SimplePlaybackManager::_StopAudio()
{
	if (!fAudioProducer)
		return;

	if (fAudioTimeSource && fAudioTimeSource != fTimeSource)
		fAudioTimeSource->Release();
	fAudioTimeSource = fTimeSource;

	fAudioProducer->SetRunning(false);

	BMediaRoster* roster = BMediaRoster::Roster();
	if (!roster)
		return;

	if (roster->Lock()) {
		// begin mucking with the media roster
ldebug("  stopping audio producer...\n");
		roster->StopNode(fAudioConnection.producer, 0, true); // synchronous stop
ldebug("  all nodes stopped\n");
		// done mucking with the media roster
		roster->Unlock();
	}
ldebug("NodeManager::_StopNodes() done\n");
}

// #pragma mark -

// _FrameGenerator
void
SimplePlaybackManager::_FrameGenerator()
{
	// start somewhere in the middle of available buffers
	// so that we are ahead of the display thread
	uint32 currentBuffer = fBufferCount / 2;
	int32 previousBuffer = -1;

	while (!fQuitting) {
		if (fPaused) {
			snooze(9000);
			continue;
		}

		// lock the buffer for rendering, so that we wait in
		// case it is currently displayed
		fBufferLock[currentBuffer].Lock();

		if (previousBuffer >= 0) {
			// release previous buffer after the next
			// one has been locked, to prevent the
			// display thread to pass us
			fBufferLock[previousBuffer].Unlock();
		}

		bigtime_t generateStartTime = system_time();

		// we can potentially load a new playlist
		// NOTE: this is the only code path resulting in the fPlaylist pointer
		// being changed, so we do not need locking
		if (!fPlaylist)
			_SwitchPlaylistIfNecessary();

		// attach painter to the buffer and clear it
		BBitmapBuffer buffer(fOverlayBitmap[currentBuffer]);
		fPainter.MemoryDestinationChanged(&buffer);
			// we are going to attach again further below, but
			// but we may actually be rendering directly into
			// this bitmap, and not into an offscreen cache.
		bigtime_t blankTime = generateStartTime;
		if (!fPlaylist) {
			fPainter.ClearBuffer();
			blankTime = system_time();
		}

		// generate frame
		fTimeSource->Lock();

		bigtime_t estimatedDisplayRealTime
			= (bigtime_t)(fLastDisplayedFrameRealtime + (fFrameCountSinceStart
				- fLastDisplayedFrame)
				* fTimeSource->TimePerVideoFrame(fFrameRateScale));

		fTimeSource->Unlock();

		bigtime_t estimatedPerformanceTime
			= fAudioTimeSource->PerformanceTimeFor(estimatedDisplayRealTime)
				- fPerformanceStartTime;
		double frameSincePlaybackStart
			= estimatedPerformanceTime * VideoFramesPerSecond() / 1000000;

		frameSincePlaybackStart -= fLastPlaylistSwitchFrame;
		frameSincePlaybackStart += fPlaylistStartFrameOffset;

		if (fPlaylist) {
			fCurrentFrame = frameSincePlaybackStart
				- floor(frameSincePlaybackStart / fPlaylist->Duration())
					* fPlaylist->Duration();
//printf("frameSincePlaybackStart: %.3f, fCurrentFrame: %.3f\n",
//	frameSincePlaybackStart, fCurrentFrame);

			// the next call will need a write lock, we should not
			// have the read lock already!
			_CurrentFrameChanged(fCurrentFrame);

			if (!fLocker->ReadLock()) {
				// this is probably dumb... if the lock
				// failed, we might as well exit
				previousBuffer = currentBuffer;
				currentBuffer++;
				if (currentBuffer == fBufferCount)
					currentBuffer = 0;
				snooze(9000);
				continue;
			}

			// copy the playlist while holding the lock
			RenderPlaylist renderPlaylist(*fPlaylist, fCurrentFrame,
				fOverlayBitmap[currentBuffer]->ColorSpace(), &fRendererCache);

			fLocker->ReadUnlock();

			if (!fHurryUp) {
				// clear the buffer only where needed
				BRegion cleanBG(fPainter.Bounds());
				renderPlaylist.RemoveSolidRegion(&cleanBG, &fPainter,
					fCurrentFrame);
				fPainter.ClearBuffer(cleanBG);
				blankTime = system_time();
	
				renderPlaylist.Generate(&fPainter, fCurrentFrame);
// visualizing a problem with switching overlays reliably - we seem
// to skip some buffers from time to time, but independent of seemingly
// correct timing calculations:
//fPainter.SetColor(255, 0, 0);
//fPainter.SetPenSize(2);
//fPainter.StrokeLine(BPoint(currentBuffer * 4, 0), BPoint(currentBuffer * 4, 50));
			}
// replace the rendering above with this to see what the background region is
//fPainter->ClearBuffer();
//int32 rectCount = cleanBG.CountRects();
//fPainter->SetColor(255, 0, 0);
//for (int32 i = 0; i < rectCount; i++) {
//	fPainter->FillRect(cleanBG.RectAt(i));
//}
		} else {
			fCurrentFrame = frameSincePlaybackStart;
		}

		fRendererCache.DeleteOldRenderers();

		fPlaybackFrame[currentBuffer] = fFrameCountSinceStart++;

		// flush content to overlay buffer
		bigtime_t copyTime = system_time();

		// we're likely flushing the caches into an overlay bitmap,
		// so we need to lock it (it was ok not to hold the lock
		// until now, since we were rendering to the main memory cache
		// anyways)
		if (fOverlayBitmap[currentBuffer]->LockBits() == B_OK) {
			// NOTE: After acquiring the over lock, the memory destination
			// may have changed (relocation in graphics memory because of
			// mode switching and such)
			fPainter.MemoryDestinationChanged(&buffer);
			fPainter.FlushCaches();
			fOverlayBitmap[currentBuffer]->UnlockBits();
		}

		// remember which buffer was locked so it
		// is released in the next loop
		previousBuffer = currentBuffer;

		currentBuffer++;
		if (currentBuffer == fBufferCount)
			currentBuffer = 0;

		// keep track of performance and incremenent
		// the total number of generated frames
		bigtime_t finish = system_time();
		fCopyTime += finish - copyTime;
		fBlankTime += blankTime - generateStartTime;
		fGenerateTime += finish - generateStartTime;
		fFrameCount++;
	}

	if (previousBuffer >= 0)
		fBufferLock[previousBuffer].Unlock();
}

//// _FrameDisplayer
//void
//SimplePlaybackManager::_FrameDisplayer()
//{
//	bigtime_t timeForNextFrame = system_time();
//	uint32 currentBuffer = 0;
//	int32 previousBuffer = -1;
//
//	BScreen screen(B_MAIN_SCREEN_ID);
//	bool waitForRetraceSupported = screen.WaitForRetrace() >= B_OK;
//
//	while (!fQuitting) {
//		if (fPaused) {
//			snooze_until(timeForNextFrame, B_SYSTEM_TIMEBASE);
//		} else {
//			if (waitForRetraceSupported) {
//				// wait until next frame time
//				screen.WaitForRetrace();
//			} else {
//				snooze_until(timeForNextFrame, B_SYSTEM_TIMEBASE);
//			}
//
//			// lock the next buffer
//			fBufferLock[currentBuffer].Lock();
//			// display it
//			if (!_SetOverlay(fOverlayBitmap[currentBuffer])) {
//				// fatal error
//				fBufferLock[currentBuffer].Unlock();
//				break;
//			}
//
//			if (previousBuffer >= 0) {
//				// release previous buffer during the vertical
//				// blank, so that we can assume the new overlay
//				// bitmap has been set
//				if (waitForRetraceSupported) {
//					screen.WaitForRetrace();
//				} else {
//					// wait for at least a bit, we just
//					// hope that the buffer is switched then
//					snooze(10000);
//				}
//				fBufferLock[previousBuffer].Unlock();
//			}
//
//			// swap buffers
//			previousBuffer = currentBuffer;
//			currentBuffer++;
//			if (currentBuffer == fBufferCount)
//				currentBuffer = 0;
//		}
//#if 0
//		if (fFrameCount % 3 == 0)
//			timeForNextFrame += 33334;
//		else
//			timeForNextFrame += 33333;
//#else
//		timeForNextFrame += 40000;
//#endif
//	}
//
//	if (previousBuffer >= 0)
//		fBufferLock[previousBuffer].Unlock();
//}

// _FrameDisplayer
void
SimplePlaybackManager::_FrameDisplayer()
{
	uint32 currentBuffer = 0;
	int32 previousBuffer = -1;
	int32 beforePreviousBuffer = -1;

	int32 framesDropped = 0;

	while (!fQuitting) {
		if (fPaused) {
			snooze(10000);
		} else {
			// lock the next buffer
			fBufferLock[currentBuffer].Lock();

			int64 performanceFrame = fPlaybackFrame[currentBuffer];

			fTimeSource->Lock();

			// TODO: imprecise!
			double timePerVideoFrame = fTimeSource->TimePerVideoFrame(
				fFrameRateScale);
			fLastDisplayedFrameRealtime = (bigtime_t)(fRealStartTime
				+ performanceFrame * timePerVideoFrame);
			fLastDisplayedFrame = performanceFrame;

//bigtime_t check = system_time();
//if (fLastDisplayedFrameRealtime + 200 < check) {
//printf("calculated snooze_until() in the past (diff: %lld)!\n",
//	fLastDisplayedFrameRealtime + 200 - check);
//fTimeSource->PrintToStream();
//} else {
//printf("  waiting: %lld\n",
//	fLastDisplayedFrameRealtime + 200 - check);
//}

			fTimeSource->Unlock();

			bigtime_t now = system_time();
			bool tooLate = (fLastDisplayedFrameRealtime + 500) < now;
				// a tiny bit too late is ok

			// TODO: When we realize that we are too late (the wakeup time
			// is in the past) we start dropping frames by not switching the
			// overlay. We still push the locked buffers along though as if
			// we would be displaying them. The side effect of this is that
			// the frame for which we hold the lock is not the truely displayed
			// frame. So the frame generator can render into the buffer
			// currently being displayed.
			// We do this in order to not block the generator thread.

			if (!tooLate) {
				fHurryUp = false;
					// reset the hurry up flag before waiting
				snooze_until(fLastDisplayedFrameRealtime + 200,
							 B_SYSTEM_TIMEBASE);

				now = system_time();
				// display it
				if (!_SetOverlay(fOverlayBitmap[currentBuffer])) {
					print_warning("failed to set overlay - "
						"quitting playback loop\n");
					fDisplayTime += system_time() - now;
					// fatal error
					fBufferLock[currentBuffer].Unlock();
					break;
				}
//				if (framesDropped >= 1) {
//					print_warning("dropped %ld frames in order to catch up\n",
//						framesDropped);
//				}
				framesDropped = 0;
			} else {
				fHurryUp = true;
//				if (framesDropped == 0)
//					print_warning("dropping frames: wakeup: %lld, too late: %lld\n",
//						fLastDisplayedFrameRealtime + 100, now - (fLastDisplayedFrameRealtime + 100));
//				if ((framesDropped % 60) == 0) {
//					print_warning("too late by %lld usecs\n",
//						now - (fLastDisplayedFrameRealtime + 200));
//				}
				framesDropped++;
			}

			if (beforePreviousBuffer >= 0) {
// NOTE: no need to do this anymore, because we're releasing the
// buffer *before* the previous buffer, but I keep this code here
// to remind of the problem that once you SetViewOverlay() a bitmap,
// you can't be sure that the previous bitmap is not showing anymore
//				// release previous buffer during the vertical
//				// blank, so that we can assume the new overlay
//				// bitmap has been set
//				snooze_until(fLastDisplayedFrameRealtime
//					+ (bigtime_t)(timePerVideoFrame / 2),
//						 B_SYSTEM_TIMEBASE);
				fBufferLock[beforePreviousBuffer].Unlock();
			}

			// swap buffers
			beforePreviousBuffer = previousBuffer;
			previousBuffer = currentBuffer;
			currentBuffer++;
			if (currentBuffer == fBufferCount)
				currentBuffer = 0;
		}
	}

	if (beforePreviousBuffer >= 0)
		fBufferLock[beforePreviousBuffer].Unlock();

	if (previousBuffer >= 0)
		fBufferLock[previousBuffer].Unlock();
}

// #pragma mark -

// _SetOverlay
bool
SimplePlaybackManager::_SetOverlay(BBitmap* bitmap)
{
	if (fQuitting)
		return false;

	if (fVideoView->LockLooperWithTimeout(100000) < B_OK)
		return false;

	if (bitmap->ColorSpace() == B_YCbCr422) {
		if (fClearViewWithOverlayColor) {
			fClearViewWithOverlayColor = false;
			fVideoView->SetHighColor(fVideoView->ViewColor());
			fVideoView->FillRect(fVideoView->Bounds());
			fVideoView->Sync();
		}

		rgb_color key;
		fVideoView->SetViewOverlay(bitmap, bitmap->Bounds(),
			fVideoView->Bounds(), &key, B_FOLLOW_ALL,
			 B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL
				| B_OVERLAY_TRANSFER_CHANNEL);
	} else {
// enable the code to mark the bitmap in the left top corner
// to see wether it is drawn at all (black bitmap versus no bitmap).
//uint8* bits = (uint8*)bitmap->Bits();
//bits[0] = 255;
//bits[1] = 255;
//bits[2] = 255;
		fVideoView->DrawBitmap(bitmap, bitmap->Bounds(), fVideoView->Bounds());
	}

	fVideoView->UnlockLooper();

	return true;
}


// #pragma mark -


void
SimplePlaybackManager::_PlaybackStarted()
{
	if (!fListenersLock.Lock())
		return;

	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaybackListener* listener
			= (PlaybackListener*)listeners.ItemAtFast(i);
		listener->PlayModeChanged(MODE_PLAYING_FORWARD);
	}

	fListenersLock.Unlock();
}


void
SimplePlaybackManager::_PlaybackStopped()
{
	if (!fListenersLock.Lock())
		return;

	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaybackListener* listener
			= (PlaybackListener*)listeners.ItemAtFast(i);
		listener->PlayModeChanged(MODE_PLAYING_PAUSED_FORWARD);
	}

	fListenersLock.Unlock();
}


void
SimplePlaybackManager::_CurrentFrameChanged(double currentFrame)
{
	if (!fListenersLock.Lock())
		return;

	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaybackListener* listener
			= (PlaybackListener*)listeners.ItemAtFast(i);
		listener->CurrentFrameChanged(currentFrame);
	}

	fListenersLock.Unlock();
}

void
SimplePlaybackManager::_SwitchPlaylistIfNecessary()
{
	if (!fListenersLock.Lock())
		return;

	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		PlaybackListener* listener
			= (PlaybackListener*)listeners.ItemAtFast(i);
		listener->SwitchPlaylistIfNecessary();
	}

	fListenersLock.Unlock();
}

// #pragma mark -

// _PrintAvailableOverlayColorspaces
void
SimplePlaybackManager::_PrintAvailableOverlayColorspaces(BRect bounds)
{
	BBitmap* testYCbCr420 = new BBitmap(bounds,
		B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL,
		B_YCbCr420);
	if (testYCbCr420->InitCheck() == B_OK)
		print_info("  B_YCbCr420 overlay supported: yes\n");
	else
		print_info("  B_YCbCr420 overlay supported: no\n");
	delete testYCbCr420;

	BBitmap* testYCbCr444 = new BBitmap(bounds,
		B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL,
		B_YCbCr444);
	if (testYCbCr444->InitCheck() == B_OK)
		print_info("  B_YCbCr444 overlay supported: yes\n");
	else
		print_info("  B_YCbCr444 overlay supported: no\n");
	delete testYCbCr444;

	BBitmap* testRGB32 = new BBitmap(bounds,
		B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL,
		B_RGB32);
	if (testRGB32->InitCheck() == B_OK)
		print_info("  B_RGB32 overlay supported: yes\n");
	else
		print_info("  B_RGB32 overlay supported: no\n");
	delete testRGB32;
}

// #pragma mark -

// _FrameGeneratorEntry
int32
SimplePlaybackManager::_FrameGeneratorEntry(void* cookie)
{
	SimplePlaybackManager* pm = (SimplePlaybackManager*)cookie;
	pm->_FrameGenerator();
	return 0;
}

// _FrameDisplayerEntry
int32
SimplePlaybackManager::_FrameDisplayerEntry(void* cookie)
{
	SimplePlaybackManager* pm = (SimplePlaybackManager*)cookie;
	pm->_FrameDisplayer();
	return 0;
}


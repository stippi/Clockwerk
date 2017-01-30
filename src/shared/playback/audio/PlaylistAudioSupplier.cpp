/*
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#include "PlaylistAudioSupplier.h"

#include <algorithm>
#include <new>
#include <string.h>

#include "AudioResampler.h"
#include "PlaybackManagerInterface.h"
#include "Playlist.h"
#include "PlaylistAudioReader.h"

// debugging
#include "Debug.h"
//#define ldebug debug
#define ldebug nodebug

using std::nothrow;


struct PlayingInterval {
								PlayingInterval(bigtime_t startTime,
												bigtime_t endTime) {
									start_time = startTime;
									end_time = endTime;
								}

			bigtime_t			start_time;
			bigtime_t			end_time;
			bigtime_t			x_start_time;
			bigtime_t			x_end_time;
			float				speed;
};


// constructor
PlaylistAudioSupplier::PlaylistAudioSupplier(Playlist* playlist,
		RWLocker* locker, PlaybackManagerInterface* playbackManager,
		float videoFrameRate)
	: AudioSupplier(),
	  fPlaylist(NULL),
	  fLocker(locker),
	  fPlaybackManager(playbackManager),
	  fAudioReader(NULL),
	  fAudioResampler(NULL),
	  fVideoFrameRate(videoFrameRate)
{
	SetPlaylist(playlist);
}

// destructor
PlaylistAudioSupplier::~PlaylistAudioSupplier()
{
	SetPlaylist(NULL);

	delete fAudioReader;
	delete fAudioResampler;
}

// GetFrames
status_t
PlaylistAudioSupplier::GetFrames(void* buffer, int64 frameCount,
								 bigtime_t startTime, bigtime_t endTime)
{
ldebug("PlaylistAudioSupplier::GetFrames(%p, %Ld, %Ld, %Ld)\n",
buffer, frameCount, startTime, endTime);
	// Create a list of playing intervals which compose the supplied
	// performance time interval.
	BList playingIntervals;
	status_t error = fPlaybackManager->LockWithTimeout(10000);
	if (error == B_OK) {
		bigtime_t intervalStartTime = startTime;
		while (intervalStartTime < endTime) {
			PlayingInterval* interval
				= new (nothrow) PlayingInterval(intervalStartTime, endTime);
			if (!interval) {
				error = B_NO_MEMORY;
				break;
			}
			fPlaybackManager->GetPlaylistTimeInterval(
				interval->start_time, interval->end_time,
				interval->x_start_time, interval->x_end_time,
				interval->speed);
			if (!playingIntervals.AddItem(interval)) {
				delete interval;
				error = B_NO_MEMORY;
				break;
			}
			intervalStartTime = interval->end_time;
		}
		fPlaybackManager->SetCurrentAudioTime(endTime);
		fPlaybackManager->Unlock();
	} else if (error == B_TIMED_OUT) {
		ldebug("  PlaylistAudioSupplier: LOCKING THE PLAYBACK MANAGER TIMED "
			   "OUT!!!\n");
	}
	// Retrieve the audio data for each interval.
	int64 framesRead = 0;
	while (!playingIntervals.IsEmpty()) {
		PlayingInterval* interval
			= (PlayingInterval*)playingIntervals.RemoveItem((int32)0);
		if (error != B_OK) {
			delete interval;
			continue;
		}
ldebug("  interval [%Ld, %Ld): [%Ld, %Ld)\n",
interval->start_time, interval->end_time,
interval->x_start_time, interval->x_end_time);
		// get playing direction
		int32 playingDirection = 0;
		if (interval->speed > 0)
			playingDirection = 1;
		else if (interval->speed < 0)
			playingDirection = -1;
		float absSpeed = interval->speed * playingDirection;
		int64 framesToRead = AudioFrameForTime(interval->end_time)
			- AudioFrameForTime(interval->start_time);
		// not playing
		if (absSpeed == 0)
			_ReadSilence(buffer, framesToRead);
		// playing
		else {
			fAudioResampler->SetInOffset(
				AudioFrameForTime(interval->x_start_time));
			fAudioResampler->SetTimeScale(absSpeed);
			error = fAudioResampler->Read(buffer, 0, framesToRead);
			// backwards -> reverse frames
			if (error == B_OK && interval->speed < 0)
				_ReverseFrames(buffer, framesToRead);
		}
		// read silence on error
		if (error != B_OK) {
			_ReadSilence(buffer, framesToRead);
			error = B_OK;
		}
		framesRead += framesToRead;
		buffer = _SkipFrames(buffer, framesToRead);
		delete interval;
	}
	// read silence on error
	if (error != B_OK) {
		_ReadSilence(buffer, frameCount);
		error = B_OK;
	}
ldebug("PlaylistAudioSupplier::GetFrames() done\n");
	return error;
}

// SetFormat
void
PlaylistAudioSupplier::SetFormat(const media_format& format)
{
	delete fAudioReader;
	delete fAudioResampler;
	fAudioReader = new (nothrow) PlaylistAudioReader(fPlaylist, fLocker,
		format, fVideoFrameRate);
	fAudioResampler = new (nothrow) AudioResampler(fAudioReader,
		format.u.raw_audio.frame_rate, 1.0);
}

// Format
const media_format&
PlaylistAudioSupplier::Format() const
{
	return fAudioReader->Format();
}

// InitCheck
status_t
PlaylistAudioSupplier::InitCheck() const
{
	status_t error = AudioSupplier::InitCheck();
	if (error == B_OK && (!fAudioReader || !fAudioResampler))
		error = B_NO_INIT;
	return error;
}

// SetPlaylist
void
PlaylistAudioSupplier::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	Playlist* oldList = fPlaylist;
	fPlaylist = playlist;

	if (fPlaylist)
		fPlaylist->Acquire();

	if (fAudioReader)
		fAudioReader->SetPlaylist(fPlaylist);
	if (oldList)
		oldList->Release();
}

// SetVolume
void
PlaylistAudioSupplier::SetVolume(float percent)
{
	fAudioReader->SetVolume(percent);
}

// AudioFrameForVideoFrame
int64
PlaylistAudioSupplier::AudioFrameForVideoFrame(int64 frame) const
{
	return fAudioReader->AudioFrameForVideoFrame(frame);
}

// AudioFrameForTime
int64
PlaylistAudioSupplier::AudioFrameForTime(bigtime_t time) const
{
	return (int64)((double)time * (double)Format().u.raw_audio.frame_rate
		/ 1000000.0);
}

// VideoFrameForAudioFrame
int64
PlaylistAudioSupplier::VideoFrameForAudioFrame(int64 frame) const
{
	return fAudioReader->VideoFrameForAudioFrame(frame);
}

// VideoFrameForTime
int64
PlaylistAudioSupplier::VideoFrameForTime(bigtime_t time) const
{
	return (int64)((double)time * (double)fVideoFrameRate / 1000000.0);
}

// _ReadSilence
void
PlaylistAudioSupplier::_ReadSilence(void* buffer, int64 frames) const
{
//ldebug("PlaylistAudioSupplier::_ReadSilence(%p, %Ld): clearing %ld bytes\n",
//buffer, frames, (char*)_SkipFrames(buffer, frames) - (char*)buffer);
	memset(buffer, 0, (char*)_SkipFrames(buffer, frames) - (char*)buffer);
}

// _ReverseFrames
void
PlaylistAudioSupplier::_ReverseFrames(void* buffer, int64 frames) const
{
	int32 sampleSize = Format().u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 frameSize = sampleSize * Format().u.raw_audio.channel_count;
	char* front = (char*)buffer;
	char* back = (char*)buffer + (frames - 1) * frameSize;
	while (front < back) {
		for (int32 i = 0; i < frameSize; i++)
			std::swap(front[i], back[i]);
		front += frameSize;
		back -= frameSize;
	}
}

// _SkipFrames
void*
PlaylistAudioSupplier::_SkipFrames(void* buffer, int64 frames) const
{
	int32 sampleSize = Format().u.raw_audio.format
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 frameSize = sampleSize * Format().u.raw_audio.channel_count;
	return (char*)buffer + frames * frameSize;
}

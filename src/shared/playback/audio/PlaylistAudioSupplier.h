/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#ifndef PLAYLIST_AUDIO_SUPPLIER_H
#define PLAYLIST_AUDIO_SUPPLIER_H

#include <List.h>

#include "AudioSupplier.h"

class AudioResampler;
class PlaybackManagerInterface;
class PlaylistAudioReader;
class Playlist;
class RWLocker;

class PlaylistAudioSupplier : public AudioSupplier {
 public:
								PlaylistAudioSupplier(Playlist* playlist,
									RWLocker* locker,
									PlaybackManagerInterface* playbackManager,
									float videoFrameRate);
	virtual						~PlaylistAudioSupplier();

	virtual	status_t			GetFrames(void* buffer, int64 frameCount,
									bigtime_t startTime, bigtime_t endTime);

	virtual	void				SetFormat(const media_format& format);
	virtual	const media_format&	Format() const;

	virtual	status_t			InitCheck() const;

			int64				AudioFrameForVideoFrame(int64 frame) const;
			int64				AudioFrameForTime(bigtime_t time) const;
			int64				VideoFrameForAudioFrame(int64 frame) const;
			int64				VideoFrameForTime(bigtime_t time) const;

			void				SetPlaylist(Playlist* playlist);

			void				SetVolume(float percent);

 private:
			void				_ReadSilence(void* buffer, int64 frames) const;

			void				_ReverseFrames(void* buffer,
									int64 frames) const;
			void*				_SkipFrames(void* buffer, int64 frames) const;

 private:
			Playlist*			fPlaylist;
			RWLocker*			fLocker;
			PlaybackManagerInterface* fPlaybackManager;
			PlaylistAudioReader* fAudioReader;
			AudioResampler*		fAudioResampler;
			float				fVideoFrameRate;
};

#endif	// PLAYLIST_AUDIO_SUPPLIER_H

/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

// This AudioReader provides linear access to an Playlist's audio data.

#ifndef XSHEET_AUDIO_READER_H
#define XSHEET_AUDIO_READER_H

#include <List.h>

#include "AudioReader.h"

class AudioAdapter;
class AudioMixer;
class SoundRegistry;
class Playlist;
class RWLocker;

class PlaylistAudioReader : public AudioReader {
 public:
								PlaylistAudioReader(Playlist* playlist,
									RWLocker* locker,
									const media_format& format,
									float videoFrameRate);
	virtual						~PlaylistAudioReader();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			void				SetPlaylist(Playlist* playlist);
			Playlist*			Source() const;

			int64				AudioFrameForVideoFrame(int64 frame) const;
			int64				VideoFrameForAudioFrame(int64 frame) const;

			void				SetVolume(float percent);

 protected:
			void				_GetActiveItemsAtFrame(Playlist* playlist,
									int64 videoFrame, int64 levelZeroOffset,
									int32 recursionLevel);

			Playlist*			fPlaylist;
			RWLocker*			fLocker;
			float				fVideoFrameRate;
			BList				fSoundItems;
			BList				fAudioReaders;
			AudioAdapter*		fAdapter;
			AudioMixer*			fMixer;
};

#endif	// XSHEET_AUDIO_READER_H

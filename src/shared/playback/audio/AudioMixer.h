/*
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include <List.h>

#include "AudioReader.h"

class AudioMixer : public AudioReader {
 public:
								AudioMixer(const media_format& format);
	virtual						~AudioMixer();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			void				AddSource(AudioReader* source);
			int32				CountSources() const;
//			void				MakeSourceListEmpty();
			AudioReader*		RemoveSource(int32 index);
			AudioReader*		SourceAt(int32 index) const;

			void				SetVolume(float percent);

 protected:
			BList				fSources;
			float				fVolumeFactor;
};

#endif	// AUDIO_MIXER_H

/*
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

// This AudioReader does both resampling an audio source to a different
// sample rate and rescaling the time, e.g. it is possible to convert the
// source data from 41.1 KHz to 96 KHz played backward twice as fast
// (time scale = -2).

#ifndef AUDIO_RESAMPLER_H
#define AUDIO_RESAMPLER_H

#include "AudioReader.h"

class AudioResampler : public AudioReader {
 public:
								AudioResampler(AudioReader* source,
											   float frameRate,
											   float timeScale = 1.0);
	virtual						~AudioResampler();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			AudioReader*		Source() const;

			void				SetFrameRate(float frameRate);
			void				SetTimeScale(float timeScale);
			float				FrameRate() const;
			float				TimeScale() const;

			void				SetInOffset(int64 offset);
			int64				InOffset() const;

			int64				ConvertFromSource(int64 pos) const;
			int64				ConvertToSource(int64 pos) const;

 private:
			status_t			_ReadLinear(void* buffer, int64 pos,
											int64 frames);

 private:
			AudioReader*		fSource;
			float				fTimeScale;	// speed
			int64				fInOffset;
};

#endif	// AUDIO_RESAMPLER_H

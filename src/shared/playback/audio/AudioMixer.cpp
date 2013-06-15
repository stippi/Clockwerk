/*
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#include <math.h>
#include <string.h>

#include "AudioMixer.h"

// debugging
#include "Debug.h"
//#define ldebug debug
#define ldebug nodebug

// turn on EITHER!
#define LOW_QUALITY 0
#define NO_CLICKS 1

//---------------------------------------------------------------
// Utility functions for mixing
//---------------------------------------------------------------

#define s 0.3f
#define t 1.5f

static inline float
h(float x)
{
	return 1.0f - pow((x - 1.0f), 2.0f);
}

static inline float
g(float x)
{
	return s + (1.0f - pow(((x - s) / (t - s)) - 1.0f, 2.0f)) * (1.0f - s);
}

static inline float
f(float x)
{
	if (x < -s)
		return -f(-x);
	if (-s <= x && x <= s)
		return x;
	if (s < x && x < 1.0f)
		return (1.0f - h((x - s) / (1.0f - s))) * x + h((x - s) / (1.0f - s)) * g(x);
	if (1.0f <= x && x < t)
		return g(x);
//	if (t <= x)
		return 1.0f;
}

// constructor
AudioMixer::AudioMixer(const media_format& format)
	: AudioReader(format),
	  fSources(10),
	  fVolumeFactor(1.0)
{
	// adjust the format according to our needs
	fFormat.type = B_MEDIA_RAW_AUDIO;
	fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	if (fFormat.u.raw_audio.channel_count >= 2)
		fFormat.u.raw_audio.channel_count = 2;
	else
		fFormat.u.raw_audio.channel_count = 1;
	fFormat.u.raw_audio.byte_order
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
//	fFormat.u.raw_audio.buffer_size = ;
}

// destructor
AudioMixer::~AudioMixer()
{
}

// Read
status_t
AudioMixer::Read(void* buffer, int64 pos, int64 frames)
{
ldebug("AudioMixer::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);
	pos += fOutOffset;
	// clear the buffer first
	ReadSilence(buffer, frames);
	if (CountSources() == 0) {
		// no sources, we're done
ldebug("AudioMixer::Read() done\n");
		return B_OK;
	}
	// find out what's the maximum number of channels of our sources
	uint32 maxChannels = 1;
	for (int32 i = 0; AudioReader* source = SourceAt(i); i++) {
		if (source->Format().u.raw_audio.channel_count > maxChannels)
			maxChannels = source->Format().u.raw_audio.channel_count;
	}
	// alloc a buffer large enough to store the data of the source with
	// the most channels
	float* sourceData = new float[frames * maxChannels];
	// read each source and cummulate the sum in the output buffer
	uint32 outChannelCount = fFormat.u.raw_audio.channel_count;
	uint32 cummulatedChannels = 0;
	for (int32 i = 0; AudioReader* source = SourceAt(i); i++) {
		if (source->Read(sourceData, pos, frames) == B_OK) {
			float* outBuffer = (float*)buffer;
			float* sourceBuffer = sourceData;
			uint32 channelCount = source->Format().u.raw_audio.channel_count;
			if (outChannelCount == 1) {
				if (channelCount == 1) {
					// one channel (mono)
					for (int64 frame = 0; frame < frames; frame++) {
						// center
						*outBuffer += *sourceBuffer;
						outBuffer++;
						sourceBuffer++;
					}
				} else {
					// more than one channel (at least stereo) -- consider
					// the first two channels (left and right) only.
					for (int64 frame = 0; frame < frames; frame++) {
						// left and right
						*outBuffer += (sourceBuffer[0] + sourceBuffer[1]) / 2;
						outBuffer++;
						sourceBuffer += channelCount;
					}
				}
			} else if (outChannelCount == 2) {
				if (channelCount == 1) {
					// one channel (mono) -- add it to left and right
					for (int64 frame = 0; frame < frames; frame++) {
						// left
						*outBuffer += *sourceBuffer;
						outBuffer++;
						// right
						*outBuffer += *sourceBuffer;
						outBuffer++;
						sourceBuffer++;
					}
				} else {
					// more than one channel (at least stereo) -- consider
					// the first two channels (left and right) only.
					uint32 skipChannels = channelCount - 1;
					for (int64 frame = 0; frame < frames; frame++) {
						// left
						*outBuffer += *sourceBuffer;
						outBuffer++;
						sourceBuffer++;
						// right
						*outBuffer += *sourceBuffer;
						outBuffer++;
						sourceBuffer += skipChannels;
					}
				}
			} // else: can't happen
			cummulatedChannels++;
		}
	}
	delete[] sourceData;
	// normalize the cummulated data and apply volume factor
	float* outBuffer = (float*)buffer;
	int64 samples = frames * outChannelCount;
#if LOW_QUALITY
	if (cummulatedChannels > 1) {
		float normalize = 1.0f / (float)cummulatedChannels;
		for (int64 i = 0; i < samples; i++) {
			*outBuffer *= normalize * fVolumeFactor;
			outBuffer++;
		}
	}
#elif NO_CLICKS
	if (cummulatedChannels > 1) {
		for (int64 i = 0; i < samples; i++) {
			*outBuffer = f(*outBuffer) * fVolumeFactor;
			outBuffer++;
		}
	}
#else
	if (false) {
	}
#endif
	else if (fVolumeFactor != 1.0) {
		for (int64 i = 0; i < samples; i++) {
			*outBuffer *= fVolumeFactor;
			outBuffer++;
		}
	}
ldebug("AudioMixer::Read() done\n");
	return B_OK;
}

// InitCheck
status_t
AudioMixer::InitCheck() const
{
	return AudioReader::InitCheck();
}

// AddSource
void
AudioMixer::AddSource(AudioReader* source)
{
	if (source)
		fSources.AddItem(source);
}

// CountSources
int32
AudioMixer::CountSources() const
{
	return fSources.CountItems();
}

// RemoveSource
AudioReader*
AudioMixer::RemoveSource(int32 index)
{
	return (AudioReader*)fSources.RemoveItem(index);
}

// SourceAt
AudioReader*
AudioMixer::SourceAt(int32 index) const
{
	return (AudioReader*)fSources.ItemAt(index);
}

// #pragma mark -

// SetVolume
void
AudioMixer::SetVolume(float percent)
{
	fVolumeFactor = percent / 100.0;
}


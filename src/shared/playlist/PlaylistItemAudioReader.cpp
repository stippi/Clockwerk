/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistItemAudioReader.h"

#include <stdio.h>

#include <MediaTrack.h>

#include "CommonPropertyIDs.h"
#include "PlaylistItem.h"
#include "PropertyAnimator.h"

// constructor
PlaylistItemAudioReader::PlaylistItemAudioReader(PlaylistItem* item,
		AudioReader* reader)
	: AudioReader(reader->Format())
	, fSource(reader)
	, fItem(item)

	, fCurrentGainIndex(0)
	, fLastGain(1.0)
{
	for (int32 i = 0; i < MAX_GAIN_VALUES; i++)
		fCurrentGains[i] = 1.0;
}

// destructor
PlaylistItemAudioReader::~PlaylistItemAudioReader()
{
	delete fSource;
}

// calc_possible_gain
template<typename sample>
static
float
calc_possible_gain(sample* buffer, uint32 channelCount, int32 frames,
	sample minSampleValue, sample maxSampleValue)
{
	sample minSample = maxSampleValue;
	sample maxSample = minSampleValue;
	int32 count = frames * channelCount;
	for (int32 i = 0; i < count; i++, buffer++) {
		if (*buffer < minSample)
			minSample = *buffer;
		if (*buffer > maxSample)
			maxSample = *buffer;
	}

	float maxSampleF = max_c(fabs((float)minSample), fabs((float)maxSample));

	float maxGain = 1.0;
	if (maxSampleF != 0.0)
		maxGain = (float)maxSampleValue / maxSampleF;
	return maxGain;
}

// scale_fixed
template<typename sample>
static
void
scale_fixed(sample* buffer, uint32 channelCount, int32 frames, float scale)
{
	int32 count = frames * channelCount;
	for (int32 i = 0; i < count; i++, buffer++)
		*buffer = (sample)(*buffer * scale);
}

// scale_linear
template<typename sample>
static
void
scale_linear(sample* buffer, uint32 channelCount, int32 frames,
			 float startScale, float endScale)
{
	float scaleDiff = endScale - startScale;
	for (int32 i = 0; i < frames; i++) {
		float scale = startScale + scaleDiff * i / frames;
		for (uint32 c = 0; c < channelCount; c++, buffer++)
			*buffer = (sample)(*buffer * scale);
	}
}

// Read
status_t
PlaylistItemAudioReader::Read(void* buffer, int64 pos, int64 frames)
{
//printf("PlaylistItemAudioReader::Read(%p, %lld, %lld)\n", buffer, pos, frames);

	pos += OutOffset();
	status_t ret = fSource->Read(buffer, pos, frames);
	if (ret < B_OK)
		return ret;

	uint32 channelCount = fFormat.u.raw_audio.channel_count;

	float possibleGain = 1.0;
	float maxAutoGain = fItem->Value(PROPERTY_MAX_AUTO_GAIN, (float)1.0);

	// calculate the max possible gain for this buffer
	if (maxAutoGain > 1.0) {
		switch (fFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				possibleGain = calc_possible_gain((float*)buffer,
					channelCount, (int32)frames, (float)-1.0, (float)+1.0);
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				possibleGain = calc_possible_gain((int32*)buffer,
					channelCount, (int32)frames, (int32)INT32_MIN, (int32)INT32_MAX);
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				possibleGain = calc_possible_gain((int16*)buffer,
					channelCount, (int32)frames, (int16)-32768, (int16)32767);
				break;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				// TODO: broken, because middle is 128
				break;
			case media_raw_audio_format::B_AUDIO_CHAR:
				possibleGain = calc_possible_gain((int8*)buffer,
					channelCount, (int32)frames, (int8)-128, (int8)+127);
				break;
		}
	}

	// write the current possible gain in one of the measurement slots
	if (possibleGain < 1.0) {
		// should not be possible, since it means that
		// that a sample value was out of range!
		if (int(possibleGain * 100) < 100)
			printf("possible gain = %.3f!\n", possibleGain);
		possibleGain = 1.0;
	}
	if (possibleGain > maxAutoGain)
		possibleGain = maxAutoGain;

	fCurrentGains[fCurrentGainIndex++] = possibleGain;
	if (fCurrentGainIndex >= MAX_GAIN_VALUES)
		fCurrentGainIndex = 0;

	// smooth out the wanted gain by iterating over all measurement slots
	// to calculate it, this will delay the change
	float wantedGain = 0.0;
	for (int32 i = 0; i < MAX_GAIN_VALUES; i++) {
		wantedGain += fCurrentGains[i];
	}
	wantedGain /= MAX_GAIN_VALUES;
	wantedGain = min_c(possibleGain, wantedGain);
	float lastGain = min_c(possibleGain, fLastGain);
	fLastGain = wantedGain;

//printf("using gain: %.2f\n", wantedGain);

	PropertyAnimator* animator = fItem->AlphaAnimator();
	if (!animator)
		return B_OK;

	float videoFPS = fItem->VideoFramesPerSecond();
	float audioFPS = fFormat.u.raw_audio.frame_rate;
	float itemStartFrame = pos * videoFPS / audioFPS;
	float itemEndFrame = (pos + frames) * videoFPS / audioFPS;
	// get the "scale/alpha/whatever" at those frames
	float startScale = animator->ScaleAtFloat(itemStartFrame) * lastGain;
	float endScale = animator->ScaleAtFloat(itemEndFrame) * wantedGain;
	if (startScale == 1.0 && endScale == 1.0)
		return B_OK;
	else if (startScale == 0.0 && endScale == 0.0)
		ReadSilence(buffer, frames);
	else if (startScale == endScale) {
		// choose the sample buffer to be used
		switch (fFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				scale_fixed((float*)buffer,
					channelCount, (int32)frames, startScale);
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				scale_fixed((int32*)buffer,
					channelCount, (int32)frames, startScale);
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				scale_fixed((int16*)buffer,
					channelCount, (int32)frames, startScale);
				break;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				// TODO: broken, because middle is 128
				scale_fixed((uint8*)buffer,
					channelCount, (int32)frames, startScale);
				break;
			case media_raw_audio_format::B_AUDIO_CHAR:
				scale_fixed((int8*)buffer,
					channelCount, (int32)frames, startScale);
				break;
		}
	} else {
		// choose the sample buffer to be used
		switch (fFormat.u.raw_audio.format) {
			case media_raw_audio_format::B_AUDIO_FLOAT:
				scale_linear((float*)buffer,
					channelCount, (int32)frames, startScale, endScale);
				break;
			case media_raw_audio_format::B_AUDIO_INT:
				scale_linear((int32*)buffer,
					channelCount, (int32)frames, startScale, endScale);
				break;
			case media_raw_audio_format::B_AUDIO_SHORT:
				scale_linear((int16*)buffer,
					channelCount, (int32)frames, startScale, endScale);
				break;
			case media_raw_audio_format::B_AUDIO_UCHAR:
				// TODO: broken, because middle is 128
				scale_linear((uint8*)buffer,
					channelCount, (int32)frames, startScale, endScale);
				break;
			case media_raw_audio_format::B_AUDIO_CHAR:
				scale_linear((int8*)buffer,
					channelCount, (int32)frames, startScale, endScale);
				break;
		}
	}
	return B_OK;
}

// InitCheck
status_t
PlaylistItemAudioReader::InitCheck() const
{
	return fItem && fSource ? fSource->InitCheck() : B_NO_INIT;
}

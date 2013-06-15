/*
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include <algorithm>
#include <string.h>

#include "AudioReader.h"

// constructor
AudioReader::AudioReader()
	: fFormat(),
	  fOutOffset(0)
{
}

// constructor
AudioReader::AudioReader(const media_format& format)
	: fFormat(format),
	  fOutOffset(0)
{
}

// destructor
AudioReader::~AudioReader()
{
}

// InitCheck
status_t
AudioReader::InitCheck() const
{
	return B_OK;
}

// Format
const media_format&
AudioReader::Format() const
{
	return fFormat;
}

// SetOutOffset
void
AudioReader::SetOutOffset(int64 offset)
{
	fOutOffset = offset;
}

// OutOffset
int64
AudioReader::OutOffset() const
{
	return fOutOffset;
}

// FrameForTime
int64
AudioReader::FrameForTime(bigtime_t time) const
{
	double frameRate = fFormat.u.raw_audio.frame_rate;
	return int64(double(time) * frameRate / 1000000.0);
}

// TimeForFrame
bigtime_t
AudioReader::TimeForFrame(int64 frame) const
{
	double frameRate = fFormat.u.raw_audio.frame_rate;
	return bigtime_t(double(frame) * 1000000.0 / frameRate);
}

// helper function for ReadSilence()
template<typename sample_t>
inline void
fill_buffer(void* buffer, int32 count, sample_t value)
{
	sample_t* buf = (sample_t*)buffer;
	sample_t* bufferEnd = buf + count;
	for (; buf < bufferEnd; buf++)
		*buf = value;
}

// ReadSilence
//
// Fills the supplied buffer with /frames/ frames of silence and returns a
// pointer to the frames after the filled range.
// /frames/ must be >= 0.
void*
AudioReader::ReadSilence(void* buffer, int64 frames) const
{
	void* bufferEnd = SkipFrames(buffer, frames);
	int32 sampleCount = frames * fFormat.u.raw_audio.channel_count;
	switch (fFormat.u.raw_audio.format) {
		case media_raw_audio_format::B_AUDIO_FLOAT:
			fill_buffer(buffer, sampleCount, (float)0);
			break;
		case media_raw_audio_format::B_AUDIO_INT:
			fill_buffer(buffer, sampleCount, (int)0);
			break;
		case media_raw_audio_format::B_AUDIO_SHORT:
			fill_buffer(buffer, sampleCount, (short)0);
			break;
		case media_raw_audio_format::B_AUDIO_UCHAR:
			fill_buffer(buffer, sampleCount, (uchar)128);
			break;
		case media_raw_audio_format::B_AUDIO_CHAR:
			fill_buffer(buffer, sampleCount, (uchar)0);
			break;
		default:
			memset(buffer, 0, (char*)bufferEnd - (char*)buffer);
			break;
 	}
	return bufferEnd;
}

// SkipFrames
//
// Returns a buffer pointer offset by /frames/ frames.
void*
AudioReader::SkipFrames(void* buffer, int64 frames) const
{
	int32 sampleSize = fFormat.u.raw_audio.format
					   & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	return (char*)buffer + frames * frameSize;
}

// ReverseFrames
void
AudioReader::ReverseFrames(void* buffer, int64 frames) const
{
	int32 sampleSize = fFormat.u.raw_audio.format
					   & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int32 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	char* front = (char*)buffer;
	char* back = (char*)buffer + (frames - 1) * frameSize;
	while (front < back) {
		for (int32 i = 0; i < frameSize; i++)
			std::swap(front[i], back[i]);
		front += frameSize;
		back -= frameSize;
	}
}


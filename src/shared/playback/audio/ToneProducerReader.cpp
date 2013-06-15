/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#include <math.h>
#include <stdio.h>

#include <MediaTrack.h>

#include "ToneProducerReader.h"

// constructor
ToneProducerReader::ToneProducerReader(float frequency,
		const media_format& format)
	: AudioReader(format),
	  fFrequency(frequency),
	  fLastPos(0)
{
	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	fFormat.type = B_MEDIA_RAW_AUDIO;
	fFormat.u.raw_audio.byte_order = hostByteOrder;
	fFormat.u.raw_audio.format = media_raw_audio_format::B_AUDIO_FLOAT;
	fFormat.u.raw_audio.channel_count = 1;
	if (fFormat.u.raw_audio.frame_rate == 0)
		fFormat.u.raw_audio.frame_rate = 44100.0;
}

// destructor
ToneProducerReader::~ToneProducerReader()
{
}

// Read
status_t
ToneProducerReader::Read(void* _buffer, int64 pos, int64 frames)
{
	float* buffer = (float*)_buffer;
	float scale = 2 * M_PI * fFrequency / fFormat.u.raw_audio.frame_rate;

	for (int64 i = 0; i < frames; i++)
		*buffer++ = sinf((pos + i) * scale);

	if (fLastPos != pos)
		printf("discontinuity: %lld\n", pos - fLastPos);
	fLastPos = pos + frames;

	return B_OK;
}

// InitCheck
status_t
ToneProducerReader::InitCheck() const
{
	return AudioReader::InitCheck();		
}

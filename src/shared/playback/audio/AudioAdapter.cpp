/*
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#include <algorithm>

#include <ByteOrder.h>

#include "AudioAdapter.h"
#include "AudioConverter.h"
#include "AudioResampler.h"

// debugging
#include "Debug.h"
//#define ldebug debug
#define ldebug nodebug


// constructor
AudioAdapter::AudioAdapter(AudioReader* source, const media_format& format)
	: AudioReader(format),
	  fSource(source),
	  fConverter(NULL),
	  fResampler(NULL)
{
	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	fFormat.u.raw_audio.byte_order = hostByteOrder;
	if (source && source->Format().type == B_MEDIA_RAW_AUDIO) {
		fFormat.u.raw_audio.channel_count = source->Format().u.raw_audio.channel_count;
		if (fFormat.u.raw_audio.format != source->Format().u.raw_audio.format
			|| source->Format().u.raw_audio.byte_order != hostByteOrder) {
			fConverter = new AudioConverter(source, fFormat.u.raw_audio.format,
											hostByteOrder);
			source = fConverter;
		}
		if (fFormat.u.raw_audio.frame_rate
				!= source->Format().u.raw_audio.frame_rate) {
			fResampler = new AudioResampler(source,
											fFormat.u.raw_audio.frame_rate);
		}
	} else
		fSource = NULL;
}

// destructor
AudioAdapter::~AudioAdapter()
{
	delete fConverter;
	delete fResampler;
}

// Read
status_t
AudioAdapter::Read(void* buffer, int64 pos, int64 frames)
{
ldebug("AudioAdapter::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);
	status_t error = InitCheck();
	if (error != B_OK)
		return error;
	pos += fOutOffset;
	if (fResampler) {
ldebug("fResampler->Read()\n");
		return fResampler->Read(buffer, pos, frames);
	} else if (fConverter) {
ldebug("fConverter->Read()\n");
		return fConverter->Read(buffer, pos, frames);
	}
ldebug("fSource->Read()\n");
	return fSource->Read(buffer, pos, frames);
ldebug("AudioAdapter::Read() done\n");
}

// InitCheck
status_t
AudioAdapter::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && !fSource)
		error = B_NO_INIT;
	if (error == B_OK && fConverter)
		error = fConverter->InitCheck();
	if (error == B_OK && fResampler)
		error = fResampler->InitCheck();
	return error;
}

// Source
AudioReader*
AudioAdapter::Source() const
{
	return fSource;
}


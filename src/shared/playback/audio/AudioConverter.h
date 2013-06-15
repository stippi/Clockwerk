/*
 * Copyright 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

// This AudioReader just converts the source sample format (and byte order)
// into another one, e.g. LE short -> BE float. Frame rate and channel
// count remain unchanged.

#ifndef AUDIO_CONVERTER_H
#define AUDIO_CONVERTER_H

#include "AudioReader.h"

class AudioConverter : public AudioReader {
 public:
								AudioConverter(AudioReader* source,
											   uint32 format,
											   uint32 byte_order);
	virtual						~AudioConverter();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			AudioReader*		Source() const;

 protected:
			AudioReader*		fSource;
};

#endif	// AUDIO_CONVERTER_H

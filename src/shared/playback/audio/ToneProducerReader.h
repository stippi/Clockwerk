/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */

#ifndef TONE_PRODUCER_READER_H
#define TONE_PRODUCER_READER_H

#include "AudioReader.h"

class ToneProducerReader : public AudioReader {
 public:
								ToneProducerReader(float frequency,
												   const media_format& format);
	virtual						~ToneProducerReader();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

 private:
			float				fFrequency;
			int64				fLastPos;
};

#endif	// TONE_PRODUCER_READER_H

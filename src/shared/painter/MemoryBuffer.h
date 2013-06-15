/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MEMORY_BUFFER_H
#define MEMORY_BUFFER_H

#include "RenderingBuffer.h"

class MemoryBuffer : public RenderingBuffer {
 public:
								MemoryBuffer(uint32 width, uint32 height,
											 pixel_format format,
											 uint32 bytesPerRow);
	virtual						~MemoryBuffer();

	virtual	status_t			InitCheck() const;

	virtual	pixel_format		PixelFormat() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

 private:
			uint8*				fBuffer;
			pixel_format		fFormat;
			uint32				fWidth;
			uint32				fHeight;
			uint32				fBytesPerRow;
};

#endif // MEMORY_BUFFER_H

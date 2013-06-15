/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MEDIA_RENDERING_BUFFER_H
#define MEDIA_RENDERING_BUFFER_H

#include "RenderingBuffer.h"

struct media_format;

class MediaRenderingBuffer : public RenderingBuffer {
 public:
								MediaRenderingBuffer(void* buffer,
													 const media_format* format);
	virtual						~MediaRenderingBuffer();

	virtual	status_t			InitCheck() const;

	virtual	pixel_format		PixelFormat() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

 private:
			void*				fBuffer;
			const media_format*	fFormat;
};

#endif // MEDIA_RENDERING_BUFFER_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MediaRenderingBuffer.h"

#include <MediaDefs.h>

// constructor
MediaRenderingBuffer::MediaRenderingBuffer(void* buffer,
										   const media_format* format)
	: fBuffer(buffer),
	  fFormat(format)
{
}

// destructor
MediaRenderingBuffer::~MediaRenderingBuffer()
{
}

// InitCheck
status_t
MediaRenderingBuffer::InitCheck() const
{
	return fBuffer ? B_OK : B_NO_INIT;
}

// PixelFormat
pixel_format
MediaRenderingBuffer::PixelFormat() const
{
	return (pixel_format)fFormat->u.raw_video.display.format;
}

// Bits
void*
MediaRenderingBuffer::Bits() const
{
	return fBuffer;
}

// BytesPerRow
uint32
MediaRenderingBuffer::BytesPerRow() const
{
	return fFormat->u.raw_video.display.bytes_per_row;
}

// Width
uint32
MediaRenderingBuffer::Width() const
{
	return fFormat->u.raw_video.display.line_width;
}

// Height
uint32
MediaRenderingBuffer::Height() const
{
	return fFormat->u.raw_video.display.line_count;
}


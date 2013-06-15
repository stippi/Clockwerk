/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MemoryBuffer.h"

#include <new>

using std::nothrow;

// constructor
MemoryBuffer::MemoryBuffer(uint32 width, uint32 height,
						   pixel_format format,
						   uint32 bytesPerRow)
	: fBuffer(NULL),
	  fFormat(format),
	  fWidth(width),
	  fHeight(height),
	  fBytesPerRow(bytesPerRow)
{
	if (height * bytesPerRow > 0)
		fBuffer = new (nothrow) uint8[height * bytesPerRow];
}

// destructor
MemoryBuffer::~MemoryBuffer()
{
	delete[] fBuffer;
}

// InitCheck
status_t
MemoryBuffer::InitCheck() const
{
	return fBuffer ? B_OK : B_NO_MEMORY;
}

// PixelFormat
pixel_format
MemoryBuffer::PixelFormat() const
{
	return fFormat;
}

// Bits
void*
MemoryBuffer::Bits() const
{
	return fBuffer;
}

// BytesPerRow
uint32
MemoryBuffer::BytesPerRow() const
{
	return fBytesPerRow;
}

// Width
uint32
MemoryBuffer::Width() const
{
	return fWidth;
}

// Height
uint32
MemoryBuffer::Height() const
{
	return fHeight;
}


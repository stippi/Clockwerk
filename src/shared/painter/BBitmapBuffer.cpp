/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "BBitmapBuffer.h"

#include <Bitmap.h>

// constructor
BBitmapBuffer::BBitmapBuffer(const BBitmap* bitmap)
	: fBitmap(bitmap)
{
}

// destructor
BBitmapBuffer::~BBitmapBuffer()
{
}

// InitCheck
status_t
BBitmapBuffer::InitCheck() const
{
	status_t ret = B_NO_INIT;
	if (fBitmap)
		ret = fBitmap->InitCheck();
	return ret;
}

// PixelFormat
pixel_format
BBitmapBuffer::PixelFormat() const
{
	return (pixel_format)fBitmap->ColorSpace();
}

// Bits
void*
BBitmapBuffer::Bits() const
{
	return fBitmap->Bits();
}

// BytesPerRow
uint32
BBitmapBuffer::BytesPerRow() const
{
	return fBitmap->BytesPerRow();
}

// Width
uint32
BBitmapBuffer::Width() const
{
	return fBitmap->Bounds().IntegerWidth() + 1;
}

// Height
uint32
BBitmapBuffer::Height() const
{
	return fBitmap->Bounds().IntegerHeight() + 1;
}


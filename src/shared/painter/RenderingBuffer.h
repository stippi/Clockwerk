/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef RENDERING_BUFFER_H
#define RENDERING_BUFFER_H

#include <GraphicsDefs.h>
#include <Rect.h>


typedef enum {
	NO_FORMAT	= 0x0000,		// B_NO_COLOR_SPACE
	BGR32		= 0x0008,		// B_RGB32
	BGRA32		= 0x2008,		// B_RGBA32
	YCbCr422	= 0x4000,		// B_YCbCr422
	YCbCr444	= 0x4003,		// B_YCbCr444
	YCbCrA		= 0x6003,		// not defined in GraphicsDefs.h
} pixel_format;


class RenderingBuffer {
 public:
								RenderingBuffer() {}
	virtual						~RenderingBuffer() {}

	virtual	status_t			InitCheck() const = 0;

	virtual	pixel_format		PixelFormat() const = 0;
	virtual	void*				Bits() const = 0;
	virtual	uint32				BytesPerRow() const = 0;
	// the *count* of the pixels per line
	virtual	uint32				Width() const = 0;
	// the *count* of lines
	virtual	uint32				Height() const = 0;

	inline	uint32				BitsLength() const
									{ return Height() * BytesPerRow(); }

	inline	BRect				Bounds() const
									{ return BRect(0.0f, 0.0f,
										(float)(Width() - 1),
										(float)(Height() - 1)); }

			void				PrintToStream() const;
};

#endif // RENDERING_BUFFER_H

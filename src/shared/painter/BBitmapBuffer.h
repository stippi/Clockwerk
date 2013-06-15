/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef B_BITMAP_BUFFER_H
#define B_BITMAP_BUFFER_H

#include "RenderingBuffer.h"

class BBitmap;

class BBitmapBuffer : public RenderingBuffer {
 public:
								BBitmapBuffer(const BBitmap* bitmap);
	virtual						~BBitmapBuffer();

	virtual	status_t			InitCheck() const;

	virtual	pixel_format		PixelFormat() const;
	virtual	void*				Bits() const;
	virtual	uint32				BytesPerRow() const;
	virtual	uint32				Width() const;
	virtual	uint32				Height() const;

								// BBitmapBuffer
			const BBitmap*		Bitmap() const
									{ return fBitmap; }
 private:

	const	BBitmap*			fBitmap;
};

#endif // B_BITMAP_BUFFER_H

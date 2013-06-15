/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIME_CODE_OVERLAY_H
#define TIME_CODE_OVERLAY_H


// Purpose:
// compose a time code string into a given bitmap
// with with adjustable alpha transparency and size

#include <Font.h>

class BBitmap;

enum {
	BITS_0			= 0,
	BITS_1			= 1,
	BITS_2			= 2,
	BITS_3			= 3,
	BITS_4			= 4,
	BITS_5			= 5,
	BITS_6			= 6,
	BITS_7			= 7,
	BITS_8			= 8,
	BITS_9			= 9,
	BITS_COLUMN		= 10,
	BITS_DOT		= 11,
};

#define	BITMAP_COUNT 12

class TimeCodeOverlay {
 public:
								TimeCodeOverlay(float fontSize,
												const BFont* font = be_bold_font);
	virtual						~TimeCodeOverlay();

			bool				IsValid() const;
								// transparency = 0.0 ... 1.0 (opaque)
			void				SetTransparency(float transparency);
			void				SetSize(float fontSize,
										const BFont* font = be_bold_font);
								// call IsValid() before calling this function!
			void				DrawTimeCode(const BBitmap* bitmap,
											 int64 frame, float fps) const;
	virtual	const BBitmap*		BitmapForDigit(uint32 which) const;
	virtual	const char*			StringForDigit(uint32 which) const;

 private:
			void				_Init(const BFont* font, float size);
			void				_BlurAlpha(BBitmap* bitmap) const;
			void				_DrawBitmap(uint8* dest, uint32 destBPR,
											color_space format, uint32 which) const;

 			BBitmap*			fBitmaps[BITMAP_COUNT];
 			float				fDigitWidth;
 			float				fWidth;
 			float				fHeight;
 			float				fTransparency;
};

#endif // TIME_CODE_OVERLAY_H

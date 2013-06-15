/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <math.h>
#include <stdio.h>

#include <Bitmap.h>
#include <View.h>

#include "TimeCodeOverlay.h"

// constructor
TimeCodeOverlay::TimeCodeOverlay(float size, const BFont* font)
	: fDigitWidth(0.0),
	  fWidth(0.0),
	  fHeight(0.0),
	  fTransparency(0.8)
{
	for (int32 i = 0; i < BITMAP_COUNT; i++)
		fBitmaps[i] = NULL;
	_Init(font, size);
}

// destructor
TimeCodeOverlay::~TimeCodeOverlay()
{
	for (int32 i = 0; i < BITMAP_COUNT; i++)
		delete fBitmaps[i];
}

// IsValid
bool
TimeCodeOverlay::IsValid() const
{
	bool valid = false;
	for (int32 i = 0; i < BITMAP_COUNT; i++) {
		valid = (fBitmaps[i] != NULL && fBitmaps[i]->IsValid());
		if (!valid)
			break;
	}
	return valid;
}

// SetTransparency
void
TimeCodeOverlay::SetTransparency(float transparency)
{
	fTransparency = transparency;
}

// SetSize
void
TimeCodeOverlay::SetSize(float size, const BFont* font)
{
	_Init(font, size);
}

// DrawTimeCode
void
TimeCodeOverlay::DrawTimeCode(const BBitmap* bitmap,
						 	  int64 frame, float fps) const
{
	if (bitmap && bitmap->IsValid()) {
		color_space format = bitmap->ColorSpace();
		if (format == B_RGB32 || format == B_RGBA32
			|| format == B_RGB24 || format == B_YCbCr422) {
			// figure out position of time code string
			int32 width = bitmap->Bounds().IntegerWidth() + 1;
			int32 height = bitmap->Bounds().IntegerHeight() + 1;
			float xPos = width / 2.0 - fWidth / 2.0;
			float yPos = height - (height / 20.0) - fHeight;
			// sanity check to see if we fit into the bitmap
			if (xPos >= 0.0 && xPos + fWidth < width
				&& yPos >= 0.0 && yPos + fHeight < height) {
				// figure out bytes per pixel for color space
				uint32 bytesPerPixel = 4;
				if (format == B_RGB24)
					bytesPerPixel = 3;
				else if (format == B_YCbCr422)
					bytesPerPixel = 2;
				uint32 digitBytes = (uint32)(fWidth / 10.0) * bytesPerPixel;
				// figure out time code for frame and fps
				uint32 seconds = frame / (int32)fps;
				uint32 minutes = seconds / 60;
				uint32 hours = minutes / 60;
				seconds -= minutes * 60;
				minutes -= hours * 60;
				uint32 frames = (int32)(frame % (int32)fps) + 1;
				// compose appropriate digit bitmaps into bitmap
				uint32 bpr = bitmap->BytesPerRow();
				uint8* bits = (uint8*)bitmap->Bits() + bytesPerPixel * (uint32)xPos
								+ (uint32)yPos * bpr;
				// draw hours
				_DrawBitmap(bits, bpr, format, hours);
				bits += digitBytes;
				// draw first :
				_DrawBitmap(bits, bpr, format, BITS_COLUMN);
				bits += digitBytes;
				// draw minutes
				uint32 minutes2 = minutes / 10;
				_DrawBitmap(bits, bpr, format, minutes2);
				bits += digitBytes;
				_DrawBitmap(bits, bpr, format, minutes - 10 * minutes2);
				bits += digitBytes;
				// draw second :
				_DrawBitmap(bits, bpr, format, BITS_COLUMN);
				bits += digitBytes;
				// draw seconds
				uint32 seconds2 = seconds / 10;
				_DrawBitmap(bits, bpr, format, seconds2);
				bits += digitBytes;
				_DrawBitmap(bits, bpr, format, seconds - 10 * seconds2);
				bits += digitBytes;
				// draw .
				_DrawBitmap(bits, bpr, format, BITS_DOT);
				bits += digitBytes;
				// draw frames
				uint32 frames2 = frames / 10;
				_DrawBitmap(bits, bpr, format, frames2);
				bits += digitBytes;
				_DrawBitmap(bits, bpr, format, frames - 10 * frames2);
			} else
				fprintf(stderr, "TimeCodeOverlay::DrawTimeCode(): Error - dest bitmap too small.\n");
		} else
			fprintf(stderr, "TimeCodeOverlay::DrawTimeCode(): Error - color space not suported.\n");
	} else
		fprintf(stderr, "TimeCodeOverlay::DrawTimeCode(): Error - dest bitmap not valid.\n");
}

// BitmapForDigit
const BBitmap*
TimeCodeOverlay::BitmapForDigit(uint32 which) const
{
	if (which < BITMAP_COUNT)
		return fBitmaps[which];
	return NULL;
}

// StringForDigit
const char*
TimeCodeOverlay::StringForDigit(uint32 index) const
{
	switch (index) {
		case BITS_0:
			return "0";
		case BITS_1:
			return "1";
		case BITS_2:
			return "2";
		case BITS_3:
			return "3";
		case BITS_4:
			return "4";
		case BITS_5:
			return "5";
		case BITS_6:
			return "6";
		case BITS_7:
			return "7";
		case BITS_8:
			return "8";
		case BITS_9:
			return "9";
		case BITS_COLUMN:
			return ":";
		case BITS_DOT:
			return ".";
	}
	return "";
}

// _Init
void
TimeCodeOverlay::_Init(const BFont* font, float size)
{
	BFont f(*font);
	f.SetSize(size);
	font_height fh;
	f.GetHeight(&fh);

	fDigitWidth = ceilf(f.StringWidth("0")) + 6.0;
	fWidth = 10.0 * (fDigitWidth - 4.0); // there are ten digits in a time code "0:00:00.00"
	fHeight = ceilf(fh.ascent + fh.descent) + 4.0;
	BRect frame(0.0, 0.0, fDigitWidth - 1.0, fHeight - 1.0);
	BBitmap temp(frame, B_RGB32, true);
	BView view(frame, "helper", B_FOLLOW_ALL, B_WILL_DRAW);
	if (temp.Lock()) {
		temp.AddChild(&view);
		view.SetLowColor(0, 0, 0, 255);
		view.SetHighColor(255, 255, 255, 255);
		view.SetFont(&f);
		// create a bitmap for every number, column and dot
		for (uint32 i = 0; i < BITMAP_COUNT; i++) {
			delete fBitmaps[i];
			// draw number into view
			view.FillRect(frame, B_SOLID_LOW);
			const char* string = StringForDigit(i);
			float pos = fDigitWidth / 2.0 - view.StringWidth(string) / 2.0;
			view.DrawString(string, BPoint(pos, fh.ascent + 2.0));
			view.Sync();
			// create bitmap by making a copy
			fBitmaps[i] = new BBitmap(&temp, false);
			if (fBitmaps[i] && fBitmaps[i]->IsValid()) {
				// compute the correct alpha channel
				_BlurAlpha(fBitmaps[i]);
			} else {
				delete fBitmaps[i];
				fBitmaps[i] = NULL;
			}
		}
		view.RemoveSelf();
		temp.Unlock();
	}
}

// _BlurAlpha
void
TimeCodeOverlay::_BlurAlpha(BBitmap* bitmap) const
{
	uint32 bpr = bitmap->BytesPerRow();
	uint32 width = bitmap->Bounds().IntegerWidth() + 1;
	uint32 height = bitmap->Bounds().IntegerHeight() + 1;
	uint8* bits = (uint8*)bitmap->Bits();
	// blur alpha channel in seperate buffer
	uint8* alphaBuffer = new uint8[width * height];
	for (uint32 y = 0; y < height; y++) {
		for (uint32 x = 0; x < width; x++) {
			// offset points to blue channel (alpha is constant at 255)
			uint32 offset = x * 4;
			if (x > 1 && x < width - 2 && y > 1 && y < height - 2) {
				uint32 blurred = bits[offset];
				// top
				blurred += bits[offset - 4] / 4;
				blurred += bits[offset - 8] / 8;
				// bottom
				blurred += bits[offset + 4] / 4;
				blurred += bits[offset + 8] / 8;
				// left
				blurred += bits[offset - bpr] / 4;
				blurred += bits[offset - 2 * bpr] / 4;
				// right
				blurred += bits[offset + bpr] / 4;
				blurred += bits[offset + 2 * bpr] / 8;
				// top left
				blurred += bits[offset - bpr - 4] / 8;
				blurred += bits[offset - bpr - 8] / 16;
				blurred += bits[offset - 2 * bpr - 4] / 16;
				// top right
				blurred += bits[offset - bpr + 4] / 8;
				blurred += bits[offset - bpr + 8] / 16;
				blurred += bits[offset - 2 * bpr + 4] / 16;
				// bottom left
				blurred += bits[offset + bpr - 4] / 8;
				blurred += bits[offset + bpr - 8] / 16;
				blurred += bits[offset + 2 * bpr - 4] / 16;
				// bottom right
				blurred += bits[offset + bpr + 4] / 8;
				blurred += bits[offset + bpr + 8] / 16;
				blurred += bits[offset + 2 * bpr + 4] / 16;

				alphaBuffer[x + y * width] = min_c(255, blurred);
			} else 
				alphaBuffer[x + y * width] = 0;
		}
		bits += bpr;
	}
	// copy blurred buffer to alpha channel
	bits = (uint8*)bitmap->Bits();
	for (uint32 y = 0; y < height; y++) {
		for (uint32 x = 0; x < width; x++) {
			// offset points to alpha channel (assumes little endian bitmap)
			uint32 offset = x * 4 + 3;
			bits[offset] = alphaBuffer[x + y * width];
		}
		bits += bpr;
	}
	delete[] alphaBuffer;
}

// _DrawBitmap
void
TimeCodeOverlay::_DrawBitmap(uint8* dest, uint32 destBPR,
							 color_space format, uint32 which) const
{
	if (const BBitmap* digit = BitmapForDigit(which)) {
		uint8* src = (uint8*)digit->Bits();
		uint32 srcBPR = digit->BytesPerRow();
		uint32 width = digit->Bounds().IntegerWidth() + 1;
		uint32 height = digit->Bounds().IntegerHeight() + 1;
		if (format == B_RGB32 || format == B_RGBA32) {
			for (uint32 y = 0; y < height; y++) {
				for (uint32 x = 0; x < width; x++) {
					uint32 offset = x * 4;
					// check alpha channel
					uint8 alpha = src[offset + 3];
					if (alpha > 0) {
						float destScale = 1.0 - (alpha / 255.0) * fTransparency;
						float srcScale = (alpha / 255.0) * fTransparency;
						dest[offset + 0] = (uint8)min_c(255, destScale * (float)dest[offset + 0]
															 + srcScale * (float)src[offset + 0]);
						dest[offset + 1] = (uint8)min_c(255, destScale * (float)dest[offset + 1]
															 + srcScale * (float)src[offset + 1]);
						dest[offset + 2] = (uint8)min_c(255, destScale * (float)dest[offset + 2]
															 + srcScale * (float)src[offset + 2]);
					}
				}
				src += srcBPR;
				dest += destBPR;
			}
		} else if (format == B_RGB24) {
			for (uint32 y = 0; y < height; y++) {
				for (uint32 x = 0; x < width; x++) {
					uint32 srcOffset = x * 4;
					uint32 dstOffset = x * 3;
					// check alpha channel
					uint8 alpha = src[srcOffset + 3];
					if (alpha > 0) {
						float destScale = 1.0 - (alpha / 255.0) * fTransparency;
						float srcScale = (alpha / 255.0) * fTransparency;
						dest[dstOffset + 0] = (uint8)min_c(255, destScale * (float)dest[dstOffset + 0]
															 + srcScale * (float)src[srcOffset + 0]);
						dest[dstOffset + 1] = (uint8)min_c(255, destScale * (float)dest[dstOffset + 1]
															 + srcScale * (float)src[srcOffset + 1]);
						dest[dstOffset + 2] = (uint8)min_c(255, destScale * (float)dest[dstOffset + 2]
															 + srcScale * (float)src[srcOffset + 2]);
					}
				}
				src += srcBPR;
				dest += destBPR;
			}
		} else if (format == B_YCbCr422) {
			for (uint32 y = 0; y < height; y++) {
				for (uint32 x = 0; x < width; x++) {
					uint32 srcOffset = x * 4;
					uint32 dstOffset = x * 2;
					// check alpha channel
					uint8 alpha = src[srcOffset + 3];
					if (alpha > 0) {
						// modify Y component only, source value needs scaling, because 16 <= Y <= 240
						float destScale = 1.0 - (alpha / 255.0) * fTransparency;
						float srcScale = (alpha / 255.0) * fTransparency;
						uint8 dstPart = uint8(destScale * (float)(dest[dstOffset] - 16));
						uint8 srcPart = uint8(srcScale * (float)(src[srcOffset] * 224 / 255));
						dest[dstOffset] = 16 + dstPart + srcPart;
					}
				}
				src += srcBPR;
				dest += destBPR;
			}
		}
	}
}

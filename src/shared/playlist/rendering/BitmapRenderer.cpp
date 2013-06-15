/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "BitmapRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <TranslationUtils.h>

#include "support_ui.h"

#include "AutoDeleter.h"
#include "BitmapClip.h"
#include "CommonPropertyIDs.h"
#include "MemoryBuffer.h"
#include "Painter.h"

using std::nothrow;

// constructor
BitmapRenderer::BitmapRenderer(ClipPlaylistItem* item,
		BitmapClip* bitmapClip, color_space renderFormat)
	: ClipRenderer(item, bitmapClip)
	, fClip(bitmapClip)
	, fBuffer(NULL)
	, fFadeMode(fClip ? fClip->FadeMode() : FADE_MODE_ALPHA)
{
	if (!fClip)
		return;
	BBitmap* bitmap = NULL;
	try {
		bitmap = BTranslationUtils::GetBitmap(fClip->Ref());
	} catch (...) {
		return;
	}

	ObjectDeleter<BBitmap> bitmapDeleter(bitmap);

	if (!bitmap || bitmap->InitCheck() < B_OK)
		return;

	pixel_format format = (pixel_format)bitmap->ColorSpace();
	uint32 width = bitmap->Bounds().IntegerWidth() + 1;
	uint32 height = bitmap->Bounds().IntegerHeight() + 1;
	uint32 bpr = bitmap->BytesPerRow();

	// NOTE: YCbCr422 is not directly supported, Painter knows
	// how to render both, if the format is either 422 or 444
	if ((pixel_format)renderFormat == YCbCr444
		|| (pixel_format)renderFormat == YCbCr422) {
		if (bitmap->ColorSpace() == B_RGBA32) {
			// we need an alpha channel
			format = YCbCrA;
			bpr = width * 4;
		} else {
			format = YCbCr444;
			bpr = ((width * 3 + 3) / 4) * 4;
		}
	}

	// see if the alpha channel is used at all
	uint8* src = (uint8*)bitmap->Bits();
	uint32 srcBPR = bitmap->BytesPerRow();


	if ((pixel_format)bitmap->ColorSpace() == BGRA32) {
		bool hasAlpha = false;
		for (uint32 y = 0; y < height; y++) {
			uint8* s = src;
			for (uint32 x = 0; x < width; x++) {
				if (s[3] < 255) {
					hasAlpha = true;
					break;
				}
				s += 4;
			}
			src += srcBPR;
		}
		// override target format by removing superfluous alpha channel
		if (!hasAlpha) {
			if (format == YCbCrA) {
				format = YCbCr444;
				bpr = ((width * 3 + 3) / 4) * 4;
			} else {
				format = BGR32;
			}
		}
	}

	fBuffer = new (nothrow) MemoryBuffer(width, height, format, bpr);

	status_t ret = fBuffer ? fBuffer->InitCheck() : B_NO_MEMORY;
	if (ret < B_OK) {
		printf("BitmapRenderer() - failed to create buffer: %s\n",
			   strerror(ret));
		return;
	}

	if (((pixel_format)bitmap->ColorSpace() == BGR32
		 || (pixel_format)bitmap->ColorSpace() == BGRA32)
		&& (format == BGR32 || format == BGRA32)) {
		// copy contents of the bitmap
		uint32 bytes = min_c(srcBPR, bpr);
		src = (uint8*)bitmap->Bits();
		uint8* dst = (uint8*)fBuffer->Bits();
		if (format == BGR32 && (pixel_format)bitmap->ColorSpace() == format) {
			for (uint32 i = 0; i < height; i++) {
				memcpy(dst, src, bytes);
				dst += bpr;
				src += srcBPR;
			}
		} else {
			for (uint32 y = 0; y < height; y++) {
				uint8* d = dst;
				uint8* s = src;
				if ((pixel_format)bitmap->ColorSpace() == BGRA32) {
					// bitmap has alpha channel, pre-multiply
					for (uint32 x = 0; x < width; x++) {
						if (s[3] < 255) {
							d[0] = (s[0] * s[3]) >> 8;
							d[1] = (s[1] * s[3]) >> 8;
							d[2] = (s[2] * s[3]) >> 8;
						} else {
							d[0] = s[0];
							d[1] = s[1];
							d[2] = s[2];
						}
						d[3] = s[3];
						d += 4;
						s += 4;
					}
				} else {
					// ignore bitmap alpha channel
					for (uint32 x = 0; x < width; x++) {
						d[0] = s[0];
						d[1] = s[1];
						d[2] = s[2];
						d[3] = 255;
						d += 4;
						s += 4;
					}
				}
				dst += bpr;
				src += srcBPR;
			}
		}
	} else {
		// convert bitmap to correct colorspace
		if (format == YCbCr444 || format == YCbCrA)
			_ConvertToYCbRr(bitmap, fBuffer);
	}
}

// destructor
BitmapRenderer::~BitmapRenderer()
{
	delete fBuffer;
}

// Generate
status_t
BitmapRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	if (!fBuffer)
		return B_NO_INIT;

	painter->SetSubpixelPrecise(false);

	BRect cropRect(fBuffer->Bounds());

	float alpha = painter->Alpha() / 255.0;
	if (fFadeMode != FADE_MODE_ALPHA && alpha < 1.0) {
		painter->SetAlpha(255);
		switch (fFadeMode) {
			case FADE_MODE_WIPE_LEFT_RIGHT:
				cropRect.right = cropRect.left + cropRect.Width() * alpha;
				break;
			case FADE_MODE_WIPE_RIGHT_LEFT:
				cropRect.left = cropRect.right - cropRect.Width() * alpha;
				break;
			case FADE_MODE_WIPE_TOP_BOTTOM:
				cropRect.bottom = cropRect.top + cropRect.Height() * alpha;
				break;
			case FADE_MODE_WIPE_BOTTOM_TOP:
				cropRect.top = cropRect.bottom - cropRect.Height() * alpha;
				break;
			default:
				break;
		}
	}

	painter->DrawBitmap(fBuffer, cropRect, cropRect);

	return B_OK;
}

// IsSolid
bool
BitmapRenderer::IsSolid(double frame) const
{
	return fBuffer && (fBuffer->PixelFormat() == BGR32
						|| fBuffer->PixelFormat() == YCbCr444);
}

// Sync
void
BitmapRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip) {
		fFadeMode = fClip->FadeMode();
	}
}

// #pragma mark -

// _ConvertToYCbRr
void
BitmapRenderer::_ConvertToYCbRr(const BBitmap* src, MemoryBuffer* dst)
{
	// convert from B_RGB32/B_RGBA32 to YCbCr444/YCbCrA
	uint32 width = src->Bounds().IntegerWidth() + 1;
	uint32 height = src->Bounds().IntegerHeight() + 1;

	uint32 srcBPR = src->BytesPerRow();
	uint32 dstBPR = dst->BytesPerRow();

	uint8* srcBits = (uint8*)src->Bits();
	uint8* dstBits = (uint8*)dst->Bits();

	if (dst->PixelFormat() == YCbCr444) {
		// source is expected to be in B_RGB32 color space
		for (uint32 y = 0; y < height; y++) {
			uint8* s = srcBits;
			uint8* d = dstBits;
			for (uint32 x = 0; x < width; x++) {
				d[0] = (8432 * s[2] + 16425 * s[1] + 3176 * s[0]) / 32768 + 16;
				d[1] = (-4818 * s[2] - 9527 * s[1] + 14345 * s[0]) / 32768 + 128;
				d[2] = (14345 * s[2] - 12045 * s[1] - 2300 * s[0]) / 32768 + 128;
				s += 4;
				d += 3;
			}
			srcBits += srcBPR;
			dstBits += dstBPR;
		}
	} else if (dst->PixelFormat() == YCbCrA) {
		// source is expected to be in B_RGBA32 color space
		// copy alpha channel as well and premultiply on the fly
		for (uint32 y = 0; y < height; y++) {
			uint8* s = srcBits;
			uint8* d = dstBits;
			for (uint32 x = 0; x < width; x++) {
				if (s[3] < 255) {
					d[0] = (((8432 * s[2] + 16425 * s[1] + 3176 * s[0]) / 32768 + 16) * s[3]) >> 8;
					d[1] = (((-4818 * s[2] - 9527 * s[1] + 14345 * s[0]) / 32768 + 128) * s[3]) >> 8;
					d[2] = (((14345 * s[2] - 12045 * s[1] - 2300 * s[0]) / 32768 + 128) * s[3]) >> 8;
				} else {
					d[0] = (8432 * s[2] + 16425 * s[1] + 3176 * s[0]) / 32768 + 16;
					d[1] = (-4818 * s[2] - 9527 * s[1] + 14345 * s[0]) / 32768 + 128;
					d[2] = (14345 * s[2] - 12045 * s[1] - 2300 * s[0]) / 32768 + 128;
				}
				d[3] = s[3];
				s += 4;
				d += 4;
			}
			srcBits += srcBPR;
			dstBits += dstBPR;
		}
	}
}



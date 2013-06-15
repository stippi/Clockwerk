/*
 * Copyright 2005-2008, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
 * rendering pipe-lines for stroke, fills, bitmap and text rendering.
 *
 */

#include <stdio.h>
#include <string.h>

#include <GraphicsDefs.h>
#include <Region.h>
#include <String.h>

#include <agg_bezier_arc.h>
#include <agg_bounding_rect.h>
#include <agg_conv_clip_polygon.h>
#include <agg_conv_curve.h>
#include <agg_conv_stroke.h>
#include <agg_ellipse.h>
#include <agg_image_accessors.h>
#include <agg_path_storage.h>
#include <agg_rounded_rect.h>
#include <agg_span_allocator.h>
#include <agg_span_image_filter_rgba.h>
#include <agg_span_interpolator_linear.h>

#include "agg_span_image_filter_ycbcr.h"
#include "agg_span_image_filter_ycbcra.h"

#include "support.h"
#include "support_ui.h"

#include "ShapeConverter.h"
#include "TextRenderer.h"

#include "Painter.h"

union pixel32 {
	uint32	data32;
	uint8	data8[4];
};

// blend_line32
static inline void
blend_line32(uint8* buffer, int32 pixels, uint8 r, uint8 g, uint8 b, uint8 a)
{
	r = (r * a) >> 8;
	g = (g * a) >> 8;
	b = (b * a) >> 8;
	a = 255 - a;

	uint8* d = buffer;

	for (int32 i = 0; i < pixels; i++) {
		d[0] = ((d[0] * a) >> 8) + b;
		d[1] = ((d[1] * a) >> 8) + g;
		d[2] = ((d[2] * a) >> 8) + r;
	}
}

// copy_bitmap_row_bgr32_copy
static inline void
copy_bitmap_row_bgr32_copy(uint8* dst, const uint8* src, int32 numPixels)
{
	memcpy(dst, src, numPixels * 4);
}

// copy_bitmap_row_bgr32_alpha
static inline void
copy_bitmap_row_bgr32_alpha(uint8* dst, const uint8* src, int32 numPixels)
{
	while (numPixels--) {
		if (src[3] == 255) {
			*(uint32*)dst = *(uint32*)src;
		} else {
			dst[0] = ((src[0] - dst[0]) * src[3] + (dst[0] << 8)) >> 8;
			dst[1] = ((src[1] - dst[1]) * src[3] + (dst[1] << 8)) >> 8;
			dst[2] = ((src[2] - dst[2]) * src[3] + (dst[2] << 8)) >> 8;
		}
		dst += 4;
		src += 4;
	}
}

// blit_ycbcr422_to_ycbcr422
static inline void
blit_ycbcr422_to_ycbcr422(uint8* dst, const uint8* src, int32 numPixels)
{
	memcpy(dst, src, numPixels * 2);
}

// blit_ycbcr444_to_ycbcr444
static inline void
blit_ycbcr444_to_ycbcr444(uint8* dst, const uint8* src, int32 numPixels)
{
	memcpy(dst, src, numPixels * 3);
}

// blit_ycbcr444_to_ycbcr422
static inline void
blit_ycbcr444_to_ycbcr422(uint8* dst, const uint8* src, int32 numPixels)
{
	while (numPixels > 1) {
		dst[0] = src[0];					// Y  0
		dst[1] = (src[1] + src[4]) >> 1;	// Cb 0+1
		dst[2] = src[3];					// Y  1
		dst[3] = (src[2] + src[5]) >> 1;	// Cr 0+1
		dst += 4;
		src += 6;
		numPixels -= 2;
	}
}

// blit_ycbcr422_to_ycbcr444
static inline void
blit_ycbcr422_to_ycbcr444(uint8* dst, const uint8* src, int32 numPixels)
{
	while (numPixels > 1) {
		dst[0] = src[0];					// Y  0
		dst[1] = src[1];					// Cb 0+1
		dst[2] = src[3];					// Cr 0+1

		dst[3] = src[2];					// Y  1
		dst[4] = src[1];					// Cb 0+1
		dst[5] = src[3];					// Cr 0+1
		dst += 6;
		src += 4;
		numPixels -= 2;
	}
}

// RasterizerGamma
//
// used to fake a global/master alpha
// for all drawing operations by faking
// the scanline opacity
struct RasterizerGamma {
 public:
    RasterizerGamma(double alpha, double gamma)
    	: fAlpha(alpha), fGamma(gamma) {}

    double operator() (double x) const
    {
        return fAlpha(fGamma(x));
    }

 private:
    agg::gamma_multiply	fAlpha;
    agg::gamma_power    fGamma;
};

// State
Painter::State::State()
	: fTransform(),
	  fColor((rgb_color){ 0, 0, 0, 255 }),
	  fGlobalAlpha(255),
	  fAlpha(255),
	  fSubpixelPrecise(false),
	  fPenSize(1.0),
	  fLineCapMode(B_BUTT_CAP),
	  fLineJoinMode(B_MITER_JOIN),
	  fMiterLimit(B_DEFAULT_MITER_LIMIT),

	  fPrevious(NULL)
{
}

Painter::State::State(State* previous)
	: fTransform(previous->fTransform),
	  fColor(previous->fColor),
	  fGlobalAlpha(previous->fGlobalAlpha),
	  fAlpha(255),
	  fSubpixelPrecise(previous->fSubpixelPrecise),
	  fPenSize(previous->fPenSize),
	  fLineCapMode(previous->fLineCapMode),
	  fLineJoinMode(previous->fLineJoinMode),
	  fMiterLimit(previous->fMiterLimit),

	  fPrevious(previous)
{
}

static const int32 kMaxDepth = 20;

// #pragma mark -

// constructor
Painter::Painter()
	: fBuffer(NULL)
	, fTempBuffer(NULL)
	, fBounds(0, 0, -1, -1)
	, fColorSpace(NO_FORMAT)

	, fPixelFormat(NULL)
	, fBaseRenderer(NULL)
	, fPixelFormatYCC(NULL)
	, fBaseRendererYCC(NULL)

	, fPixelFormatPremultiplied(NULL)
	, fBaseRendererPremultiplied(NULL)
	, fPixelFormatYCCPremultiplied(NULL)
	, fBaseRendererYCCPremultiplied(NULL)

	, fPackedScanline(NULL)
	, fUnpackedScanline(NULL)

	, fRasterizer(NULL)
	, fRenderer(NULL)
	, fRendererYCC(NULL)

	, fFontRendererBin(NULL)
	, fFontRendererYCCBin(NULL)

	, fState(new State())
	, fDepth(0)

	, fComplexShapeDepth(0)

	, fTextRenderer(new TextRenderer())
{
}

// destructor
Painter::~Painter()
{
	_MakeEmpty();

	while (PopState());
	delete fState;

	delete fTextRenderer;
}

// #pragma mark -

// AttachToBuffer
bool
Painter::AttachToBuffer(const RenderingBuffer* buffer)
{
	if (buffer && buffer->InitCheck() >= B_OK &&
		(buffer->PixelFormat() == BGRA32
		 || buffer->PixelFormat() == BGR32
		 || buffer->PixelFormat() == YCbCr422
		 || buffer->PixelFormat() == YCbCr444)) {

		// clean up previous stuff
		_MakeEmpty();

		fBuffer = new agg::rendering_buffer();
		fBuffer->attach((uint8*)buffer->Bits(), buffer->Width(),
			buffer->Height(), buffer->BytesPerRow());
		fBounds = buffer->Bounds();
		fColorSpace = buffer->PixelFormat();

		if (fColorSpace == YCbCr422 || fColorSpace == YCbCr444) {
			// YCbCr
			// use a temporary buffer in YCbCr444 format to speed
			// things up and greatly improve visual quality
			uint32 bytesPerRow = buffer->Width() * 3;
			bytesPerRow = ((bytesPerRow + 3) / 4) * 4;
				// add padding
			uint8* tempBuffer = new uint8[buffer->Height() * bytesPerRow];
			fTempBuffer = new agg::rendering_buffer();
			fTempBuffer->attach(tempBuffer,
								buffer->Width(),
								buffer->Height(),
								bytesPerRow);
			fPixelFormatYCC = new pixfmt_ycc(*fTempBuffer);
			fPixelFormatYCCPremultiplied = new pixfmt_pre_ycc(*fTempBuffer);


			fBaseRendererYCC = new renderer_base_ycc(*fPixelFormatYCC);

			fBaseRendererYCCPremultiplied = new renderer_base_pre_ycc(*fPixelFormatYCCPremultiplied);

			fRendererYCC = new renderer_type_ycc(*fBaseRendererYCC);

			fFontRendererYCCBin = new font_renderer_bin_type_ycc(*fBaseRendererYCC);
		} else {
			// RGBA
			fPixelFormat = new pixfmt(*fBuffer);
			fBaseRenderer = new renderer_base(*fPixelFormat);

			fPixelFormatPremultiplied = new pixfmt_pre(*fBuffer);
			fBaseRendererPremultiplied = new renderer_base_pre(*fPixelFormatPremultiplied);

			fRenderer = new renderer_type(*fBaseRenderer);

			fFontRendererBin = new font_renderer_bin_type(*fBaseRenderer);
		}

		fRasterizer = new rasterizer_type();
		fPackedScanline = new scanline_packed_type();
		fUnpackedScanline = new scanline_unpacked_type();

#if ALIASED_DRAWING
		fRasterizer->gamma(agg::gamma_threshold(0.5));
#else
		fRasterizer->gamma(RasterizerGamma((float)fState->fGlobalAlpha / 255.0, 1.0));
#endif
		fRasterizer->clip_box(fBounds.left, fBounds.top,
			fBounds.right + 1, fBounds.bottom + 1);

		// init the renderer colors
		_SetRendererColor(fState->fColor);
		return true;
	} else {
		if (buffer->InitCheck() < B_OK)
			fprintf(stderr, "Painter::AttachToBuffer() - buffer->InitCheck(): %s\n",
							strerror(buffer->InitCheck()));
	}
	return false;
}

// DetachFromBuffer
void
Painter::DetachFromBuffer()
{
	_MakeEmpty();
}

// MemoryDestinationChanged
bool
Painter::MemoryDestinationChanged(const RenderingBuffer* buffer)
{
	if (!fBuffer) {
		return AttachToBuffer(buffer);
	}
	if (fBounds != buffer->Bounds()
		|| fColorSpace != buffer->PixelFormat()) {
		printf("Painter::MemoryDestinationChanged() - incompatible buffer!\n");

		DetachFromBuffer();
		return AttachToBuffer(buffer);
	}

	fBuffer->attach((uint8*)buffer->Bits(), buffer->Width(), buffer->Height(),
		buffer->BytesPerRow());
	return true;
}

// ClearBuffer
void
Painter::ClearBuffer()
{
	ClearBuffer(fBounds);
}

// ClearBuffer
void
Painter::ClearBuffer(BRect area)
{
	if (!fBuffer || !area.IsValid() || !fBounds.Intersects(area))
		return;

	// clip area to buffer bounds
	area = area & fBounds;

	uint32 width = area.IntegerWidth() + 1;
	uint32 height = area.IntegerHeight() + 1;

	uint8* bits;
	uint32 bpr;
	if (fTempBuffer) {
		bits = fTempBuffer->row_ptr(0);
		bpr = fTempBuffer->stride();
	} else {
		bits = fBuffer->row_ptr(0);
		bpr = fBuffer->stride();
	}

	if (fColorSpace == YCbCr422 || fColorSpace == YCbCr444) {
		// NOTE: in YCbCrXXX color space, a temporary buffer
		// is always used in YCbCr444 format
		// offset bits to left top pixel in area
		bits += (int32)area.top * bpr + (int32)area.left * 3;

		while (height--) {
			uint8* handle = bits;
		    for (uint32 x = 0; x < width; x++) {
				handle[0] = 16;
				handle[1] = 128;
				handle[2] = 128;
				handle += 3;
			}
			// next row
			bits += bpr;
		}
	} else {
		// offset bits to left top pixel in area
		bits += (int32)area.top * bpr + (int32)area.left * 4;
		uint32 bytes = width * 4;
		while (height--) {
			memset(bits, 0, bytes);
			// next row
			bits += bpr;
		}
	}
}

// ClearBuffer
void
Painter::ClearBuffer(BRegion& area)
{
	if (!fBuffer || !area.Frame().IsValid()
		|| !fBounds.Intersects(area.Frame()))
		return;

	// clip area to buffer bounds
	BRegion bounds(fBounds);
	area.IntersectWith(&bounds);

	uint8* bits;
	uint32 bpr;
	if (fTempBuffer) {
		bits = fTempBuffer->row_ptr(0);
		bpr = fTempBuffer->stride();
	} else {
		bits = fBuffer->row_ptr(0);
		bpr = fBuffer->stride();
	}

	int32 count = area.CountRects();
	for (int32 i = 0; i < count; i++) {
		clipping_rect rect = area.RectAtInt(i);

		uint32 width = rect.right - rect.left + 1;
		uint32 height = rect.bottom - rect.top + 1;

		uint8* b = bits;
		if (fColorSpace == YCbCr422 || fColorSpace == YCbCr444) {
			// NOTE: in YCbCrXXX color space, a temporary buffer
			// is always used in YCbCr444 format
			// offset bits to left top pixel in area
			b += (int32)rect.top * bpr + (int32)rect.left * 3;

			while (height--) {
				uint8* handle = b;
			    for (uint32 x = 0; x < width; x++) {
					handle[0] = 16;
					handle[1] = 128;
					handle[2] = 128;
					handle += 3;
				}
				// next row
				b += bpr;
			}
		} else {
			// offset bits to left top pixel in area
			b += (int32)rect.top * bpr + (int32)rect.left * 4;
			uint32 bytes = width * 4;
			while (height--) {
				memset(b, 0, bytes);
				// next row
				b += bpr;
			}
		}
	}
}

// FlushCaches
void
Painter::FlushCaches()
{
//printf("Painter::FlushCaches()\n\n");
	if (!fTempBuffer || !fBuffer) {
		return;
	}

	uint8* srcRow = fTempBuffer->row_ptr(0);
	uint32 srcBPR = fTempBuffer->stride();
	uint8* dstRow = fBuffer->row_ptr(0);
	uint32 dstBPR = fBuffer->stride();
	uint32 rows = min_c(fBuffer->height(), fTempBuffer->height());
	uint32 numPixels = fBuffer->width();

	if (fColorSpace == YCbCr444) {
		// scrap buffer has the same format
		for (uint32 y = 0; y < rows; y++) {
			blit_ycbcr444_to_ycbcr444(dstRow, srcRow, numPixels);
			dstRow += dstBPR;
			srcRow += srcBPR;
		}
		return;
	}

	// B_YCbCr444 -> B_YCbCr422
	for (uint32 y = 0; y < rows; y++) {
		uint32 pixels = numPixels;
		uint8* dst = dstRow;
		uint8* src = srcRow;
		union pixel64 {
			uint64	data64;
			uint8	data8[8];
		};
		union pixel32 {
			uint32	data32;
			uint8	data8[4];
		};
		pixel64 p64;
		pixel32 p32;
		while (pixels >= 4) {
			p64.data8[0] = src[0];
			p64.data8[1] = (src[1] + src[4]) >> 1;
			p64.data8[2] = src[3];
			p64.data8[3] = (src[2] + src[5]) >> 1;
			p64.data8[4] = src[6];
			p64.data8[5] = (src[7] + src[10]) >> 1;
			p64.data8[6] = src[9];
			p64.data8[7] = (src[8] + src[11]) >> 1;
			*(uint64*)dst = p64.data64;
			dst += 8;
			src += 12;
			pixels -= 4;
		}
		while (pixels > 1) {
			p32.data8[0] = src[0];
			p32.data8[1] = (src[1] + src[4]) >> 1;
			p32.data8[2] = src[3];
			p32.data8[3] = (src[2] + src[5]) >> 1;
			*(uint32*)dst = p32.data32;
			dst += 4;
			src += 6;
			pixels -= 2;
		}
		if (pixels == 1) {
			dst[0] = src[0];
			dst[1] = src[1];
		}
		dstRow += dstBPR;
		srcRow += srcBPR;
	}
}

// #pragma mark - state

// SetTransformation
void
Painter::SetTransformation(const AffineTransform& transform)
{
	// set to supplied transformation
	fState->fTransform = transform;

	if (fState->fPrevious) {
		// and multiply with previous state transformation
		fState->fTransform.Multiply(fState->fPrevious->fTransform);
	}
}

// ResetTransformation
void
Painter::ResetTransformation()
{
	if (fState->fPrevious) {
		// reset to previous state transformation
		fState->fTransform = fState->fPrevious->fTransform;
	} else {
		fState->fTransform.Reset();
	}
}

// SetColor
void
Painter::SetColor(const rgb_color& color)
{
	if (fState->fColor != color) {
		fState->fColor = color;
		_SetRendererColor(color);
	}
}

// SetAlpha
void
Painter::SetAlpha(uint8 alpha)
{
	uint8 oldAlpha = fState->fGlobalAlpha;

	fState->fAlpha = alpha;

	if (fState->fPrevious) {
		fState->fGlobalAlpha = (fState->fPrevious->fGlobalAlpha * alpha) / 255;
	} else {
		fState->fGlobalAlpha = alpha;
	}

	if (fState->fGlobalAlpha != oldAlpha && fRasterizer)
		fRasterizer->gamma(RasterizerGamma(fState->fGlobalAlpha / 255.0, 1.0));
}

// SetSubpixelPrecise
void
Painter::SetSubpixelPrecise(bool precise)
{
	fState->fSubpixelPrecise = precise;
}

// SetPenSize
void
Painter::SetPenSize(float size)
{
	fState->fPenSize = size;
}

// SetFont
void
Painter::SetFont(const Font* font)
{
	fTextRenderer->SetFont(*font);
}

// SetFalseBoldWidth
void
Painter::SetFalseBoldWidth(float width)
{
	fTextRenderer->SetFalseBoldWidth(width);
}

// SetLineMode
void
Painter::SetLineMode(cap_mode capMode, join_mode joinMode)
{
	fState->fLineCapMode = capMode;
	fState->fLineJoinMode = joinMode;

}

// PushState
bool
Painter::PushState()
{
	if (fDepth == kMaxDepth)
		return false;

	fDepth++;
	fState = new State(fState);
	return true;
}

// PopState
bool
Painter::PopState()
{
	if (fDepth == 0)
		return false;

	uint8 oldAlpha = fState->fGlobalAlpha;
	rgb_color color = fState->fColor;

	State* next = fState;
	fState = next->fPrevious;
	delete next;
	fDepth--;

	// propagate the new settings to AGG pipeline
	if (color != fState->fColor)
		_SetRendererColor(fState->fColor);

	if (fState->fGlobalAlpha != oldAlpha && fRasterizer)
		fRasterizer->gamma(RasterizerGamma(fState->fGlobalAlpha / 255.0, 1.0));

	return true;
}

// #pragma mark -

// Color
rgb_color
Painter::Color() const
{
	return fState->fColor;
}

// GlobalAlpha
uint8
Painter::GlobalAlpha() const
{
	return fState->fGlobalAlpha;
}

// Alpha
uint8
Painter::Alpha() const
{
	return fState->fAlpha;
}

// Transformation
AffineTransform
Painter::Transformation() const
{
	return fState->fTransform;
}

// Scale
float
Painter::Scale() const
{
	return fState->fTransform.scale();
}

// #pragma mark -

// BeginComplexShape
void
Painter::BeginComplexShape()
{
	fComplexShapeDepth++;
}

// EndComplexShape
void
Painter::EndComplexShape()
{
	fComplexShapeDepth--;
	if (fComplexShapeDepth <= 0) {
		fComplexShapeDepth = 0;
		_Render();
	}
}

// #pragma mark - drawing

// StrokeLine
BRect
Painter::StrokeLine(BPoint a, BPoint b)
{
	// "false" means not to do the pixel center offset,
	// because it would mess up our optimized versions
	_FilterCoord(&a, false);
	_FilterCoord(&b, false);

	BRect touched(min_c(a.x, b.x), min_c(a.y, b.y),
				  max_c(a.x, b.x), max_c(a.y, b.y));

	// This is supposed to stop right here if we can see
	// that we're definitaly outside the clipping reagion.
	// Extending by penSize like that is not really correct,
	// but fast and only triggers unnecessary calculation
	// in a few edge cases
	touched.InsetBy(-(fState->fPenSize - 1), -(fState->fPenSize - 1));
	if (!touched.Intersects(fBounds)) {
		touched.Set(0.0, 0.0, -1.0, -1.0);
		return touched;
	}

	// do the pixel center offset here
	// TODO: make depend on fState->fPenSize!
	a.x += 0.5;
	a.y += 0.5;
	b.x += 0.5;
	b.y += 0.5;

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, b.y);

	touched = _StrokePath(path);

	return _Clipped(touched);
}

// DrawTriangle
inline BRect
Painter::DrawTriangle(BPoint pt1, BPoint pt2, BPoint pt3, bool fill) const
{
	_FilterCoord(&pt1);
	_FilterCoord(&pt2);
	_FilterCoord(&pt3);

	agg::path_storage path;

	path.move_to(pt1.x, pt1.y);
	path.line_to(pt2.x, pt2.y);
	path.line_to(pt3.x, pt3.y);

	path.close_polygon();

	if (fill)
		return _FillPath(path);
	else
		return _StrokePath(path);
}

// DrawPolygon
inline BRect
Painter::DrawPolygon(const BPoint* ptArray, int32 numPts, bool fill,
	bool closed) const
{
	if (numPts > 0) {

		agg::path_storage path;
		BPoint point = _FilterCoord(*ptArray);
		path.move_to(point.x, point.y);

		for (int32 i = 1; i < numPts; i++) {
			ptArray++;
			point = _FilterCoord(*ptArray);
			path.line_to(point.x, point.y);
		}

		if (closed)
			path.close_polygon();

		if (fill)
			return _FillPath(path);
		else
			return _StrokePath(path);
	}
	return BRect(0.0, 0.0, -1.0, -1.0);
}

// DrawBezier
BRect
Painter::DrawBezier(const BPoint* controlPoints, bool filled) const
{
	agg::path_storage curve;

	BPoint p1(controlPoints[0]);
	BPoint p2(controlPoints[1]);
	BPoint p3(controlPoints[2]);
	BPoint p4(controlPoints[3]);
	_FilterCoord(&p1);
	_FilterCoord(&p2);
	_FilterCoord(&p3);
	_FilterCoord(&p4);

	curve.move_to(p1.x, p1.y);
	curve.curve4(p1.x, p1.y,
				 p2.x, p2.y,
				 p3.x, p3.y);

	if (filled)
		curve.close_polygon();

	agg::conv_curve<agg::path_storage> path(curve);

	if (filled)
		return _FillPath(path);
	else
		return _StrokePath(path);
}

// DrawShape
BRect
Painter::DrawShape(/*const*/ BShape* shape, bool filled) const
{
	agg::path_storage path;
	ShapeConverter converter(&path);

//	// account for our view coordinate system
//	converter.ScaleBy(B_ORIGIN, fScale, fScale);
//	converter.TranslateBy(fOrigin);
//	// offset locations to center of pixels
//	converter.TranslateBy(BPoint(0.5, 0.5));

	converter.Iterate(shape);

	agg::conv_curve<agg::path_storage> curve(path);

	if (filled)
		return _FillPath(curve);
	else
		return _StrokePath(curve);
}

// StrokeRect
BRect
Painter::StrokeRect(const BRect& r) const
{
	// support invalid rects
	BPoint a(min_c(r.left, r.right), min_c(r.top, r.bottom));
	BPoint b(max_c(r.left, r.right), max_c(r.top, r.bottom));
	_FilterCoord(&a, false);
	_FilterCoord(&b, false);

	// first, try an optimized version
//	if (fState->fPenSize == 1.0) {
//		StrokeRect(r, fState->fColor);
//		return _Clipped(rect);
//	}

	if (fmodf(fState->fPenSize, 2.0) != 0.0) {
		// shift coords to center of pixels
		a.x += 0.5;
		a.y += 0.5;
		b.x += 0.5;
		b.y += 0.5;
	}

	agg::path_storage path;
	path.move_to(a.x, a.y);
	if (a.x == b.x || a.y == b.y) {
		// special case rects with one pixel height or width
		path.line_to(b.x, b.y);
	} else {
		path.line_to(b.x, a.y);
		path.line_to(b.x, b.y);
		path.line_to(a.x, b.y);
	}
	path.close_polygon();

	return _StrokePath(path);
}

//// StrokeRect
//void
//Painter::StrokeRect(const BRect& r, const rgb_color& c) const
//{
//	StraightLine(BPoint(r.left, r.top),
//				 BPoint(r.right - 1, r.top), c);
//	StraightLine(BPoint(r.right, r.top),
//				 BPoint(r.right, r.bottom - 1), c);
//	StraightLine(BPoint(r.right, r.bottom),
//				 BPoint(r.left + 1, r.bottom), c);
//	StraightLine(BPoint(r.left, r.bottom),
//				 BPoint(r.left, r.top + 1), c);
//}

// FillRect
BRect
Painter::FillRect(const BRect& r) const
{
	// support invalid rects
	BPoint a(min_c(r.left, r.right), min_c(r.top, r.bottom));
	BPoint b(max_c(r.left, r.right), max_c(r.top, r.bottom));
	_FilterCoord(&a, false);
	_FilterCoord(&b, false);

	// first, try an optimized version
	// TODO: only if !fSubpixelPrecise and if fTransformation not rotated!
//	FillRect(r, fState->fColor);
//	return _Clipped(r);

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	b.x += 1.0;
	b.y += 1.0;

	agg::path_storage path;
	path.move_to(a.x, a.y);
	path.line_to(b.x, a.y);
	path.line_to(b.x, b.y);
	path.line_to(a.x, b.y);
	path.close_polygon();

	return _FillPath(path);
}

// StrokeRoundRect
BRect
Painter::StrokeRoundRect(const BRect& r, float xRadius, float yRadius) const
{
	BPoint lt(r.left, r.top);
	BPoint rb(r.right, r.bottom);
	bool centerOffset = fState->fPenSize == 1.0;
	// TODO: use this when using _StrokePath()
	// bool centerOffset = fmodf(fState->fPenSize, 2.0) != 0.0;
	_FilterCoord(&lt, centerOffset);
	_FilterCoord(&rb, centerOffset);

	if (fState->fPenSize == 1.0) {
		agg::rounded_rect rect;
		rect.rect(lt.x, lt.y, rb.x, rb.y);
		rect.radius(xRadius, yRadius);

		return _StrokePath(rect);
	} else {
		// NOTE: This implementation might seem a little strange, but it makes
		// stroked round rects look like on R5. A more correct way would be to use
		// _StrokePath() as above (independent from fState->fPenSize).
		// The fact that the bounding box of the round rect is not enlarged
		// by fState->fPenSize/2 is actually on purpose, though one could argue it is unexpected.

		// enclose the right and bottom edge
		rb.x++;
		rb.y++;

		agg::rounded_rect outer;
		outer.rect(lt.x, lt.y, rb.x, rb.y);
		outer.radius(xRadius, yRadius);

		agg::conv_transform<agg::rounded_rect > transformedOuter(outer, fState->fTransform);

		fRasterizer->add_path(transformedOuter);

		// don't add an inner hole if the "size is negative", this avoids some
		// defects that can be observed on R5 and could be regarded as a bug.
		if (2 * fState->fPenSize < rb.x - lt.x && 2 * fState->fPenSize < rb.y - lt.y) {
			agg::rounded_rect inner;
			inner.rect(lt.x + fState->fPenSize, lt.y + fState->fPenSize,
					   rb.x - fState->fPenSize, rb.y - fState->fPenSize);
			inner.radius(max_c(0.0, xRadius - fState->fPenSize),
						 max_c(0.0, yRadius - fState->fPenSize));

			agg::conv_transform<agg::rounded_rect > transformedInner(inner, fState->fTransform);

			fRasterizer->add_path(transformedInner);
		}

		// make the inner rect work as a hole
		fRasterizer->filling_rule(agg::fill_even_odd);

		_Render();

		// reset to default
		fRasterizer->filling_rule(agg::fill_non_zero);

		return _Clipped(_BoundingBox(outer));
	}
}

// FillRoundRect
BRect
Painter::FillRoundRect(const BRect& r, float xRadius, float yRadius) const
{
	BPoint lt(r.left, r.top);
	BPoint rb(r.right, r.bottom);
	_FilterCoord(&lt, false);
	_FilterCoord(&rb, false);

	// account for stricter interpretation of coordinates in AGG
	// the rectangle ranges from the top-left (.0, .0)
	// to the bottom-right (.9999, .9999) corner of pixels
	rb.x += 1.0;
	rb.y += 1.0;

	agg::rounded_rect rect;
	rect.rect(lt.x, lt.y, rb.x, rb.y);
	rect.radius(xRadius, yRadius);

	return _FillPath(rect);
}

// DrawEllipse
BRect
Painter::DrawEllipse(BPoint center, float xRadius, float yRadius,
	bool fill) const
{
	// TODO: I think the conversion and the offset of
	// pixel centers might not be correct here, and it
	// might even be necessary to treat Fill and Stroke
	// differently, as with Fill-/StrokeRect().
	_FilterCoord(&center);

	int32 divisions = (int32)max_c(12, (xRadius + yRadius
		+ 2 * fState->fPenSize) * M_PI / 2);

	if (fill) {
		agg::ellipse path(center.x, center.y, xRadius, yRadius, divisions);

		return _FillPath(path);
	} else {
		// NOTE: This implementation might seem a little strange, but it makes
		// stroked ellipses look like on R5. A more correct way would be to use
		// _StrokePath(), but it currently has its own set of problems with narrow
		// ellipses (for small xRadii or yRadii).
		float inset = fState->fPenSize / 2.0;
		agg::ellipse inner(center.x, center.y,
						   max_c(0.0, xRadius - inset),
						   max_c(0.0, yRadius - inset),
						   divisions);
		agg::ellipse outer(center.x, center.y,
						   xRadius + inset,
						   yRadius + inset,
						   divisions);

		agg::conv_transform<agg::ellipse > transformedInner(inner, fState->fTransform);
		agg::conv_transform<agg::ellipse > transformedOuter(outer, fState->fTransform);

		fRasterizer->add_path(transformedOuter);
		fRasterizer->add_path(transformedInner);

		// make the inner ellipse work as a hole
		fRasterizer->filling_rule(agg::fill_even_odd);

		_Render();

		// reset to default
		fRasterizer->filling_rule(agg::fill_non_zero);

		return _Clipped(_BoundingBox(transformedOuter));
	}
}

// StrokeArc
BRect
Painter::StrokeArc(BPoint center, float xRadius, float yRadius,
				   float angle, float span) const
{
	_FilterCoord(&center);

	double angleRad = (angle * M_PI) / 180.0;
	double spanRad = (span * M_PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius,
		-angleRad, -spanRad);

	agg::conv_curve<agg::bezier_arc> path(arc);
	path.approximation_scale(2.0);

	return _StrokePath(path);
}

// FillArc
BRect
Painter::FillArc(BPoint center, float xRadius, float yRadius,
				 float angle, float span) const
{
	_FilterCoord(&center);

	double angleRad = (angle * M_PI) / 180.0;
	double spanRad = (span * M_PI) / 180.0;
	agg::bezier_arc arc(center.x, center.y, xRadius, yRadius,
		-angleRad, -spanRad);

	agg::conv_curve<agg::bezier_arc> segmentedArc(arc);

	agg::path_storage path;

	// build a new path by starting at the center point,
	// then traversing the arc, then going back to the center
	path.move_to(center.x, center.y);

	segmentedArc.rewind(0);
	double x;
	double y;
	unsigned cmd = segmentedArc.vertex(&x, &y);
	while (!agg::is_stop(cmd)) {
		path.line_to(x, y);
		cmd = segmentedArc.vertex(&x, &y);
	}

	path.close_polygon();

	return _FillPath(path);
}

// #pragma mark -

// DrawString
BRect
Painter::DrawString(const char* utf8String, uint32 length,
	BPoint baseLine, const escapement_delta* delta, BRect constrainRect)
{
	if (!fState->fSubpixelPrecise) {
		baseLine.x = roundf(baseLine.x);
		baseLine.y = roundf(baseLine.y);
	}

	BRect touched(0.0, 0.0, -1.0, -1.0);

	if (!fBuffer)
		return touched;

	if (constrainRect.IsValid()) {
		constrainRect = Transformation().TransformBounds(constrainRect);
		constrainRect = constrainRect & fBounds;
	} else
		constrainRect = fBounds;

	AffineTransform transform;
	transform.TranslateBy(baseLine);
	transform *= fState->fTransform;
	if (fColorSpace == YCbCr422 || fColorSpace == YCbCr444) {
		// YCbCr422 pipeline
		touched = fTextRenderer->RenderString(utf8String, length,
			fRendererYCC, fFontRendererYCCBin, fRasterizer,
			transform, constrainRect, false, delta);
	} else {
		// RGBA or RGB_ pipeline
		touched = fTextRenderer->RenderString(utf8String, length,
			fRenderer, fFontRendererBin, fRasterizer,
			transform, constrainRect, false, delta);
	}

	return _Clipped(touched);
}

// BoundingBox
BRect
Painter::BoundingBox(const char* utf8String, uint32 length,
					 BPoint baseLine, const escapement_delta* delta) const
{
	if (!fState->fSubpixelPrecise) {
		baseLine.x = roundf(baseLine.x);
		baseLine.y = roundf(baseLine.y);
	}

	AffineTransform transform;
	transform.TranslateBy(baseLine);
	transform *= fState->fTransform;

	static BRect dummy;
	return fTextRenderer->RenderString(utf8String,
									   length,
									   fRenderer,
									   fFontRendererBin,
									   fRasterizer,
									   transform, dummy, true,
									   delta);
}

// StringWidth
float
Painter::StringWidth(const char* utf8String)
{
	return StringWidth(utf8String, strlen(utf8String));
}

// StringWidth
float
Painter::StringWidth(const char* utf8String, uint32 length)
{
	return fTextRenderer->StringWidth(utf8String, length);
}

// #pragma mark -

// DrawBitmap
BRect
Painter::DrawBitmap(const RenderingBuffer* bitmap,
					BRect bitmapRect, BRect viewRect) const
{
	BRect touched = _Clipped(viewRect);

	if (bitmap && bitmap->InitCheck() >= B_OK && touched.IsValid()) {
		// the native bitmap coordinate system
		BRect actualBitmapRect(bitmap->Bounds());

		agg::rendering_buffer srcBuffer;
		srcBuffer.attach((uint8*)bitmap->Bits(),
						 bitmap->Width(),
						 bitmap->Height(),
						 bitmap->BytesPerRow());

		_DrawBitmap(srcBuffer, bitmap->PixelFormat(), actualBitmapRect, bitmapRect, viewRect);
	}
	return touched;
}

// #pragma mark - private

// _MakeEmpty
void
Painter::_MakeEmpty()
{
	delete fBuffer;
	fBuffer = NULL;
	if (fTempBuffer) {
		delete[] fTempBuffer->row_ptr(0);
			// we allocated that storage ourselves
		delete fTempBuffer;
		fTempBuffer = NULL;
	}

	fBounds.Set(0, 0, -1, -1);
	fColorSpace = NO_FORMAT;

	delete fPixelFormat;
	fPixelFormat = NULL;
	delete fBaseRenderer;
	fBaseRenderer = NULL;

	delete fPixelFormatYCC;
	fPixelFormatYCC = NULL;
	delete fBaseRendererYCC;
	fBaseRendererYCC = NULL;

	delete fPixelFormatPremultiplied;
	fPixelFormatPremultiplied = NULL;
	delete fBaseRendererPremultiplied;
	fBaseRendererPremultiplied = NULL;

	delete fPixelFormatYCCPremultiplied;
	fPixelFormatYCCPremultiplied = NULL;
	delete fBaseRendererYCCPremultiplied;
	fBaseRendererYCCPremultiplied = NULL;

	delete fPackedScanline;
	fPackedScanline = NULL;

	delete fRasterizer;
	fRasterizer = NULL;

	delete fRenderer;
	fRenderer = NULL;

	delete fRendererYCC;
	fRendererYCC = NULL;

	delete fFontRendererBin;
	fFontRendererBin = NULL;

	delete fFontRendererYCCBin;
	fFontRendererYCCBin = NULL;
}

// _FilterCoord
void
Painter::_FilterCoord(BPoint* point, bool centerOffset) const
{
	// rounding
	if (!fState->fSubpixelPrecise) {
		point->x = roundf(point->x);
		point->y = roundf(point->y);
	}
	// this code is supposed to move coordinates to the center of pixels,
	// as AGG considers (0,0) to be the "upper left corner" of a pixel,
	// but BViews are less strict on those details
	if (centerOffset) {
		point->x += 0.5;
		point->y += 0.5;
	}
}

// _FilterCoord
BPoint
Painter::_FilterCoord(const BPoint& point, bool centerOffset) const
{
	BPoint ret = point;
	_FilterCoord(&ret, centerOffset);
	return ret;
}

// _Clipped
BRect
Painter::_Clipped(const BRect& rect) const
{
	if (fBuffer)
		return fBounds & rect;
	return BRect(0, 0, -1, -1);
}

// _UpdateFont
void
Painter::_UpdateFont()
{
//	fTextRenderer->SetFont(fFont);
}

// _SetRendererColor
void
Painter::_SetRendererColor(const rgb_color& color) const
{
// NOTE: the "bin" renderers are for aliased font rendering,
// which is currently not used.
	if (fColorSpace == YCbCr422 || fColorSpace == YCbCr444) {
		if (fRendererYCC)
			fRendererYCC->color(agg::rgba(color.red / 255.0,
										  color.green / 255.0,
										  color.blue / 255.0,
										  color.alpha / 255.0));
//		if (fFontRendererYCCBin)
//			fFontRendererYCCBin->color(agg::rgba(color.red / 255.0,
//												 color.green / 255.0,
//												 color.blue / 255.0,
//												 color.alpha / 255.0));
	} else {
		if (fRenderer)
			fRenderer->color(agg::rgba(color.red / 255.0,
									   color.green / 255.0,
									   color.blue / 255.0,
									   color.alpha / 255.0));
//		if (fFontRendererBin)
//			fFontRendererBin->color(agg::rgba(color.red / 255.0,
//											  color.green / 255.0,
//											  color.blue / 255.0,
//											  color.alpha / 255.0));
	}
}

// #pragma mark -


// _DrawBitmap
void
Painter::_DrawBitmap(agg::rendering_buffer& srcBuffer, pixel_format format,
					 BRect actualBitmapRect, BRect bitmapRect, BRect viewRect) const
{
	if (!fBuffer
		|| !bitmapRect.IsValid() || !bitmapRect.Intersects(actualBitmapRect)
		|| !viewRect.IsValid()) {
		fprintf(stderr, "Painter::_DrawBitmap() - invalid source or destination rect\n");
		return;
	}

	if (!fState->fSubpixelPrecise) {
		// round off viewRect (in a way avoiding too much distortion)
		viewRect.OffsetTo(roundf(viewRect.left), roundf(viewRect.top));
		viewRect.right = roundf(viewRect.right);
		viewRect.bottom = roundf(viewRect.bottom);
	}

	double xScale = (viewRect.Width() + 1) / (bitmapRect.Width() + 1);
	double yScale = (viewRect.Height() + 1) / (bitmapRect.Height() + 1);

	if (xScale == 0.0 || yScale == 0.0)
		return;

	// compensate for the lefttop offset the actualBitmapRect might have
	bitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);

	// actualBitmapRect has the right size, but put it at B_ORIGIN too
	actualBitmapRect.OffsetBy(-actualBitmapRect.left, -actualBitmapRect.top);

	// constrain rect to passed bitmap bounds
	// and transfer the changes to the viewRect
	if (bitmapRect.left < actualBitmapRect.left) {
		float diff = actualBitmapRect.left - bitmapRect.left;
		viewRect.left += diff;
		bitmapRect.left = actualBitmapRect.left;
	}
	if (bitmapRect.top < actualBitmapRect.top) {
		float diff = actualBitmapRect.top - bitmapRect.top;
		viewRect.top += diff;
		bitmapRect.top = actualBitmapRect.top;
	}
	if (bitmapRect.right > actualBitmapRect.right) {
		float diff = bitmapRect.right - actualBitmapRect.right;
		viewRect.right -= diff;
		bitmapRect.right = actualBitmapRect.right;
	}
	if (bitmapRect.bottom > actualBitmapRect.bottom) {
		float diff = bitmapRect.right - actualBitmapRect.bottom;
		viewRect.bottom -= diff;
		bitmapRect.bottom = actualBitmapRect.bottom;
	}

	double xOffset = viewRect.left - bitmapRect.left;
	double yOffset = viewRect.top - bitmapRect.top;

	if (fColorSpace == YCbCr422 || fColorSpace == YCbCr444) {
		_DrawBitmapToYCbCr(srcBuffer, format, xOffset, yOffset, xScale, yScale, viewRect);
	} else {
		_DrawBitmapToRGB32(srcBuffer, format, xOffset, yOffset, xScale, yScale, viewRect);
	}
}

// _DrawBitmapToRGB32
void
Painter::_DrawBitmapToRGB32(agg::rendering_buffer& srcBuffer,
							pixel_format format,
							double xOffset, double yOffset,
							double xScale, double yScale, BRect viewRect) const
{
	switch (format) {
		case BGR32: {
			// maybe we can use an optimized version
			if (fState->fTransform.IsIdentity() && fState->fGlobalAlpha == 255
				&& xScale == 1.0 && yScale == 1.0) {
				_DrawBitmapNoScale(copy_bitmap_row_bgr32_copy, 4, 4,
								   srcBuffer, xOffset, yOffset, viewRect);
				return;
			}

			_DrawBitmapGeneric32(srcBuffer, xOffset, yOffset,
								 xScale, yScale, viewRect);
			break;
		}

		case BGRA32: {
			// maybe we can use an optimized version
//			if (fState->fTransform.IsIdentity() && fState->fGlobalAlpha == 255
//				&& xScale == 1.0 && yScale == 1.0) {
//				_DrawBitmapNoScale(copy_bitmap_row_bgr32_alpha, 4, 4,
//								   srcBuffer, xOffset, yOffset, viewRect);
//				return;
//			}

			_DrawBitmapGeneric32(srcBuffer, xOffset, yOffset,
								 xScale, yScale, viewRect);
			break;
		}

		default: {
			printf("Painter::_DrawBitmapToRGB32() - "
				   "unsupported colorspace: %s\n",
				   string_for_color_space((color_space)format));
			break;
		}
	}
}

// _DrawBitmapToYCbCr
void
Painter::_DrawBitmapToYCbCr(agg::rendering_buffer& srcBuffer,
							pixel_format format,
							double xOffset, double yOffset,
							double xScale, double yScale, BRect viewRect) const
{
	switch (format) {
		case YCbCr422: {
			// maybe we can use an optimized version
			if (fState->fTransform.IsIdentity() && fState->fGlobalAlpha == 255
				&& xScale == 1.0 && yScale == 1.0) {
				_DrawBitmapNoScale(blit_ycbcr422_to_ycbcr444, 2, 3,
								   srcBuffer, xOffset, yOffset, viewRect);
				return;
			}

			_DrawBitmapGenericYCbCr422(srcBuffer, xOffset, yOffset,
									   xScale, yScale, viewRect);
			break;
		}
		case YCbCr444: {
			// maybe we can use an optimized version
			if (fState->fTransform.IsIdentity() && fState->fGlobalAlpha == 255
				&& xScale == 1.0 && yScale == 1.0) {
				_DrawBitmapNoScale(blit_ycbcr444_to_ycbcr444, 3, 3,
								   srcBuffer, xOffset, yOffset, viewRect);
				return;
			}

			_DrawBitmapGenericYCbCr444(srcBuffer, xOffset, yOffset,
									   xScale, yScale, viewRect);
			break;
		}

		case YCbCrA: {
			_DrawBitmapGenericYCbCrA(srcBuffer, xOffset, yOffset,
									 xScale, yScale, viewRect);
			break;
		}

		default: {
			printf("Painter::_DrawBitmapToYCbCr() - "
				   "unsupported colorspace: %s\n",
				   string_for_color_space((color_space)format));
			break;
		}
	}
}

// _DrawBitmapNoScale
template <class F>
void
Painter::_DrawBitmapNoScale(F copyRowFunction,
							uint32 bytesPerSourcePixel,
							uint32 bytesPerDestPixel,
							agg::rendering_buffer& srcBuffer,
							int32 xOffset, int32 yOffset,
							BRect viewRect) const
{
//printf("_DrawBitmapNoScale(%.1f, %.1f -> %.1f, %.1f)\n",
//viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);
	const uint8* src = srcBuffer.row_ptr(0);
	uint32 srcBPR = srcBuffer.stride();

	// clip to the current clipping region's frame
	viewRect = viewRect & fBounds;

	int32 left = (int32)viewRect.left;
	int32 top = (int32)viewRect.top;
	int32 right = (int32)viewRect.right;
	int32 bottom = (int32)viewRect.bottom;

	// copy
	uint8* dst;
	uint32 dstBPR;
	int32 x1;
	int32 x2;
	int32 y1;
	int32 y2;
	// TODO: find a better way for this hackery:
	// if there is a temp buffer, then we're using
	// YCbCr format renderer.
	if (fTempBuffer) {
		dst = fTempBuffer->row_ptr(0);
		dstBPR = fTempBuffer->stride();
		x1 = max_c(fBaseRendererYCC->xmin(), left);
		x2 = min_c(fBaseRendererYCC->xmax(), right);
		y1 = max_c(fBaseRendererYCC->ymin(), top);
		y2 = min_c(fBaseRendererYCC->ymax(), bottom);
	} else {
		dst = fBuffer->row_ptr(0);
		dstBPR = fBuffer->stride();
		x1 = max_c(fBaseRenderer->xmin(), left);
		x2 = min_c(fBaseRenderer->xmax(), right);
		y1 = max_c(fBaseRenderer->ymin(), top);
		y2 = min_c(fBaseRenderer->ymax(), bottom);
	}

	if (x1 <= x2 && y1 <= y2) {
		uint8* dstHandle = dst + y1 * dstBPR + x1 * bytesPerDestPixel;
		const uint8* srcHandle = src + (y1 - yOffset) * srcBPR
			+ (x1 - xOffset) * bytesPerSourcePixel;

		for (; y1 <= y2; y1++) {
			copyRowFunction(dstHandle, srcHandle, x2 - x1 + 1);

			dstHandle += dstBPR;
			srcHandle += srcBPR;
		}
	}
}

// _DrawBitmapGeneric32
void
Painter::_DrawBitmapGeneric32(agg::rendering_buffer& srcBuffer,
							  double xOffset, double yOffset,
							  double xScale, double yScale,
							  BRect viewRect) const
{
	// AGG pipeline
	typedef agg::span_allocator<pixfmt::color_type> span_alloc_type;

//	typedef agg::image_accessor_clone<pixfmt> img_accessor_type;

	pixfmt pixf_img(srcBuffer);
//	img_accessor_type ia(pixf_img);

	typedef agg::span_interpolator_linear<> interpolator_type;
	typedef agg::span_image_filter_rgba_bilinear_clip<pixfmt, interpolator_type> span_gen_type;
//	typedef agg::span_image_filter_rgba_2x2<img_accessor_type, interpolator_type> span_gen_type;

	agg::trans_affine srcMatrix;
	srcMatrix *= fState->fTransform;

	agg::trans_affine imgMatrix;
	imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
	imgMatrix *= agg::trans_affine_translation(xOffset, yOffset);
	imgMatrix *= fState->fTransform;
	imgMatrix.invert();

	span_alloc_type spanAllocator;
//	agg::image_filter_bilinear filter_kernel;
//	agg::image_filter_lut filter(filter_kernel, false);

	interpolator_type interpolator(imgMatrix);

	// convert to pixel coords (versus pixel indices)
	viewRect.right++;
	viewRect.bottom++;

	// path encloses image
	agg::path_storage path;
	path.move_to(viewRect.left, viewRect.top);
	path.line_to(viewRect.right, viewRect.top);
	path.line_to(viewRect.right, viewRect.bottom);
	path.line_to(viewRect.left, viewRect.bottom);
	path.close_polygon();

	agg::conv_transform<agg::path_storage> transformedPath(path, srcMatrix);

	fRasterizer->add_path(transformedPath);

	span_gen_type spanGenerator(pixf_img, agg::rgba_pre(0, 0, 0, 0), interpolator);
//	span_gen_type spanGenerator(ia, interpolator);
//	span_gen_type spanGenerator(ia, interpolator, filter);

	agg::render_scanlines_aa(*fRasterizer,
							 *fUnpackedScanline,
							 *fBaseRendererPremultiplied,
							 spanAllocator,
							 spanGenerator);
}

// _DrawBitmapGenericYCbCr422
void
Painter::_DrawBitmapGenericYCbCr422(agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const
{
	// AGG pipeline

	// pixel format attached to video rendering buffer
	typedef agg::pixfmt_ycbcr422	pixfmt_image;
	pixfmt_image pixf_img(srcBuffer);

	// image transformation matrix
	agg::trans_affine srcMatrix;
	srcMatrix *= fState->fTransform;

	agg::trans_affine imgMatrix;
	imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
	imgMatrix *= agg::trans_affine_translation(xOffset, yOffset);
	imgMatrix *= fState->fTransform;
	imgMatrix.invert();

	// image interpolator
	typedef agg::span_interpolator_linear<> interpolator_type;
	interpolator_type interpolator(imgMatrix);

	// image accessor
	typedef agg::image_accessor_clip<pixfmt_image> source_type;
	source_type source(pixf_img, agg::ycbcra8(0, 0, 0, 0));

	// scanline allocator
	agg::span_allocator<pixfmt_image::color_type> spanAllocator;

	// image filter (nearest neighbor for 422 format)
	typedef agg::span_image_filter_ycbcr422_nn<source_type, interpolator_type> span_gen_type;
	span_gen_type spanGenerator(source, interpolator);

	// convert to pixel coords (versus pixel indices)
	viewRect.right++;
	viewRect.bottom++;

	// path encloses image
	agg::path_storage path;
	path.move_to(viewRect.left, viewRect.top);
	path.line_to(viewRect.right, viewRect.top);
	path.line_to(viewRect.right, viewRect.bottom);
	path.line_to(viewRect.left, viewRect.bottom);
	path.close_polygon();

	agg::conv_transform<agg::path_storage> transformedPath(path, srcMatrix);

	fRasterizer->reset();
	fRasterizer->add_path(transformedPath);

	agg::render_scanlines_aa(*fRasterizer,
							 *fUnpackedScanline,
							 *fBaseRendererYCCPremultiplied,
							 spanAllocator,
							 spanGenerator);
}

// _DrawBitmapGenericYCbCr444
void
Painter::_DrawBitmapGenericYCbCr444(agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const
{
//printf("_DrawBitmapGenericYCbCr444(%.2f, %.2f, (%.1f, %.1f -> %.1f, %.1f))\n",
//xScale, yScale, viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);
	// AGG pipeline

	// pixel format attached to video rendering buffer
	typedef agg::pixfmt_ycbcr444	pixfmt_image;
	pixfmt_image pixf_img(srcBuffer);

	// image transformation matrix
	agg::trans_affine srcMatrix;
	srcMatrix *= fState->fTransform;

	agg::trans_affine imgMatrix;
	imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
	imgMatrix *= agg::trans_affine_translation(xOffset, yOffset);
	imgMatrix *= fState->fTransform;
	imgMatrix.invert();

	// image interpolator
	typedef agg::span_interpolator_linear<> interpolator_type;
	interpolator_type interpolator(imgMatrix);

	// image accessor
	typedef agg::image_accessor_clip<pixfmt_image> source_type;
	source_type source(pixf_img, agg::ycbcra8(0, 0, 0, 0));

	// scanline allocator
	agg::span_allocator<pixfmt_image::color_type> spanAllocator;

	// image filter (bilinear for 444 format)
	typedef agg::span_image_filter_ycbcr444_bilinear<source_type, interpolator_type> span_gen_type;
	span_gen_type spanGenerator(source, interpolator);
//	// image filter (nearest neighbor for speed)
//	typedef agg::span_image_filter_ycbcr444_nn<source_type, interpolator_type> span_gen_type;
//	span_gen_type spanGenerator(source, interpolator);

	// convert to pixel coords (versus pixel indices)
	viewRect.right++;
	viewRect.bottom++;

	// path encloses image
	agg::path_storage path;
	path.move_to(viewRect.left, viewRect.top);
	path.line_to(viewRect.right, viewRect.top);
	path.line_to(viewRect.right, viewRect.bottom);
	path.line_to(viewRect.left, viewRect.bottom);
	path.close_polygon();

	agg::conv_transform<agg::path_storage> transformedPath(path, srcMatrix);

	fRasterizer->add_path(transformedPath);

	agg::render_scanlines_aa(*fRasterizer,
							 *fUnpackedScanline,
							 *fBaseRendererYCCPremultiplied,
							 spanAllocator,
							 spanGenerator);
}

// _DrawBitmapGenericYCbCrA
void
Painter::_DrawBitmapGenericYCbCrA(agg::rendering_buffer& srcBuffer,
								  double xOffset, double yOffset,
								  double xScale, double yScale,
								  BRect viewRect) const
{
//printf("_DrawBitmapGenericYCbCrA(%.2f, %.2f, (%.1f, %.1f -> %.1f, %.1f))\n",
//xScale, yScale, viewRect.left, viewRect.top, viewRect.right, viewRect.bottom);
	// AGG pipeline

	// pixel format attached to video rendering buffer
	typedef agg::pixfmt_ycbcra	pixfmt_image;
	pixfmt_image pixf_img(srcBuffer);

	// image transformation matrix
	agg::trans_affine srcMatrix;
	srcMatrix *= fState->fTransform;

	agg::trans_affine imgMatrix;
	imgMatrix *= agg::trans_affine_scaling(xScale, yScale);
	imgMatrix *= agg::trans_affine_translation(xOffset, yOffset);
	imgMatrix *= fState->fTransform;
	imgMatrix.invert();

	// image interpolator
	typedef agg::span_interpolator_linear<> interpolator_type;
	interpolator_type interpolator(imgMatrix);

	// image accessor
	typedef agg::image_accessor_clip<pixfmt_image> source_type;
	source_type source(pixf_img, agg::ycbcra8(0, 0, 0, 0));

	// scanline allocator
	agg::span_allocator<pixfmt_image::color_type> spanAllocator;

//	// image filter (bilinear for 444+a format)
//	typedef agg::span_image_filter_ycbcra_bilinear<source_type, interpolator_type> span_gen_type;
//	span_gen_type spanGenerator(source, interpolator);
	// image filter (nearest neighbor for speed)
	typedef agg::span_image_filter_ycbcra_nn<source_type, interpolator_type> span_gen_type;
	span_gen_type spanGenerator(source, interpolator);

	// convert to pixel coords (versus pixel indices)
	viewRect.right++;
	viewRect.bottom++;

	// path encloses image
	agg::path_storage path;
	path.move_to(viewRect.left, viewRect.top);
	path.line_to(viewRect.right, viewRect.top);
	path.line_to(viewRect.right, viewRect.bottom);
	path.line_to(viewRect.left, viewRect.bottom);
	path.close_polygon();

	agg::conv_transform<agg::path_storage> transformedPath(path, srcMatrix);

	fRasterizer->add_path(transformedPath);

	agg::render_scanlines_aa(*fRasterizer,
							 *fUnpackedScanline,
							 *fBaseRendererYCCPremultiplied,
							 spanAllocator,
							 spanGenerator);
}

//// _InvertRect32
//void
//Painter::_InvertRect32(BRect r) const
//{
//	if (fBuffer) {
//		int32 width = r.IntegerWidth() + 1;
//		for (int32 y = (int32)r.top; y <= (int32)r.bottom; y++) {
//			uint8* dst = fBuffer->row(y);
//			dst += (int32)r.left * 4;
//			for (int32 i = 0; i < width; i++) {
//				dst[0] = 255 - dst[0];
//				dst[1] = 255 - dst[1];
//				dst[2] = 255 - dst[2];
//				dst += 4;
//			}
//		}
//	}
//}
//
//// _BlendRect32
//void
//Painter::_BlendRect32(const BRect& r, const rgb_color& c) const
//{
//	if (fBuffer) {
//		uint8* dst = fBuffer->row(0);
//		uint32 bpr = fBuffer->stride();
//
//		int32 left = (int32)r.left;
//		int32 top = (int32)r.top;
//		int32 right = (int32)r.right;
//		int32 bottom = (int32)r.bottom;
//
//		// fill rects, iterate over clipping boxes
//		fBaseRenderer->first_clip_box();
//		do {
//			int32 x1 = max_c(fBaseRenderer->xmin(), left);
//			int32 x2 = min_c(fBaseRenderer->xmax(), right);
//			if (x1 <= x2) {
//				int32 y1 = max_c(fBaseRenderer->ymin(), top);
//				int32 y2 = min_c(fBaseRenderer->ymax(), bottom);
//
//				uint8* offset = dst + x1 * 4 + y1 * bpr;
//				for (; y1 <= y2; y1++) {
//					blend_line32(offset, x2 - x1 + 1, c.red, c.green, c.blue, c.alpha);
//					offset += bpr;
//				}
//			}
//		} while (fBaseRenderer->next_clip_box());
//	}
//}
//
// #pragma mark -

template<class VertexSource>
BRect
Painter::_BoundingBox(VertexSource& path) const
{
	double left = 0.0;
	double top = 0.0;
	double right = -1.0;
	double bottom = -1.0;
	uint32 pathID[1];
	pathID[0] = 0;
	agg::bounding_rect(path, pathID, 0, 1, &left, &top, &right, &bottom);
	return BRect(left, top, right, bottom);
}

// agg_line_cap_mode_for
inline agg::line_cap_e
agg_line_cap_mode_for(cap_mode mode)
{
	switch (mode) {
		case B_BUTT_CAP:
			return agg::butt_cap;
		case B_SQUARE_CAP:
			return agg::square_cap;
		case B_ROUND_CAP:
			return agg::round_cap;
	}
	return agg::butt_cap;
}

// agg_line_join_mode_for
inline agg::line_join_e
agg_line_join_mode_for(join_mode mode)
{
	switch (mode) {
		case B_MITER_JOIN:
			return agg::miter_join;
		case B_ROUND_JOIN:
			return agg::round_join;
		case B_BEVEL_JOIN:
		case B_BUTT_JOIN: // ??
		case B_SQUARE_JOIN: // ??
			return agg::bevel_join;
	}
	return agg::miter_join;
}

// _StrokePath
template<class VertexSource>
BRect
Painter::_StrokePath(VertexSource& path) const
{
	typedef agg::conv_stroke<VertexSource>						stroke_type;
	typedef agg::conv_transform<stroke_type>					transformed_stroke_type;
	typedef agg::conv_clip_polygon<transformed_stroke_type>		clipped_stroke_type;

	agg::conv_stroke<VertexSource> stroke(path);
	stroke.width(fState->fPenSize);

	// special case line width = 1 with square caps
	// this has a couple of advantages and it looks
	// like this is also the R5 behaviour.
	if (fState->fPenSize == 1.0 && fState->fLineCapMode == B_BUTT_CAP) {
		stroke.line_cap(agg::square_cap);
	} else {
		stroke.line_cap(agg_line_cap_mode_for(fState->fLineCapMode));
	}
	stroke.line_join(agg_line_join_mode_for(fState->fLineJoinMode));
	stroke.miter_limit(fState->fMiterLimit);

	transformed_stroke_type transformedStroke(stroke, fState->fTransform);

	fRasterizer->add_path(transformedStroke);

	_Render();

	BRect touched = _BoundingBox(path);
	float penSize = ceilf(fState->fPenSize / 2.0);
	touched.InsetBy(-penSize, -penSize);

	return _Clipped(touched);
}

// _FillPath
template<class VertexSource>
BRect
Painter::_FillPath(VertexSource& path) const
{
	typedef agg::conv_transform<VertexSource>					transformed_path_type;
	typedef agg::conv_clip_polygon<transformed_path_type>		clipped_path_type;

	transformed_path_type transformedPath(path, fState->fTransform);

	fRasterizer->add_path(transformedPath);

	_Render();

	return _Clipped(_BoundingBox(transformedPath));
}

// _Render
void
Painter::_Render() const
{
	if (fComplexShapeDepth > 0)
		return;

	if (fColorSpace == YCbCr422)
		agg::render_scanlines(*fRasterizer, *fPackedScanline, *fRendererYCC);
	else
		agg::render_scanlines(*fRasterizer, *fPackedScanline, *fRenderer);

	fRasterizer->reset();
}


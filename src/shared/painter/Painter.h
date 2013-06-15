/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * API to the Anti-Grain Geometry based "Painter" drawing backend. Manages
 * rendering pipe-lines for stroke, fills, bitmap and text rendering.
 *
 */

#ifndef PAINTER_H
#define PAINTER_H

#include <Rect.h>

#include "defines.h"

#include "AffineTransform.h"
#include "Font.h"
#include "RenderingBuffer.h"

class BRegion;
class TextRenderer;

class Painter {
 public:
								Painter();
	virtual						~Painter();

								// frame buffer stuff
			bool				AttachToBuffer(const RenderingBuffer* buffer);
			void				DetachFromBuffer();
			bool				MemoryDestinationChanged(
									const RenderingBuffer* buffer);

	inline	BRect				Bounds() const
									{ return fBounds; }
	inline	pixel_format		PixelFormat() const
									{ return fColorSpace; }

			void				ClearBuffer();
			void				ClearBuffer(BRect area);
			void				ClearBuffer(BRegion& area);
			void				FlushCaches();

								// object settings
			void				SetTransformation(
									const AffineTransform& transform);
			void				ResetTransformation();
			void				SetColor(const rgb_color& color);
	inline	void				SetColor(uint8 r, uint8 g, uint8 b,
									uint8 a = 255);
			void				SetAlpha(uint8 alpha);

			void				SetSubpixelPrecise(bool precise);

			void				SetPenSize(float size);
			void				SetFont(const Font* font);
			void				SetFalseBoldWidth(float width);

			void				SetLineMode(cap_mode capMode = B_BUTT_CAP,
									join_mode joinMode = B_MITER_JOIN);

			bool				PushState();
			bool				PopState();

			rgb_color			Color() const;
			uint8				GlobalAlpha() const;
			uint8				Alpha() const;
			AffineTransform		Transformation() const;
			float				Scale() const;

	// painting functions
			void				BeginComplexShape();
			void				EndComplexShape();

			BRect				StrokeLine(BPoint a, BPoint b);

			BRect				DrawTriangle(BPoint pt1, BPoint pt2,
									BPoint pt3, bool filled) const;

			BRect				DrawPolygon(const BPoint* ptArray,
									int32 numPts, bool filled,
									bool  closed = true) const;

			BRect				DrawBezier(const BPoint* controlPoints,
									bool filled) const;
	
			BRect				DrawShape(/*const*/ BShape* shape,
									bool filled) const;

								// rects
			BRect				StrokeRect(const BRect& r) const;

			BRect				FillRect(const BRect& r) const;

								// round rects
			BRect				StrokeRoundRect(const BRect& r, float xRadius,
									float yRadius) const;

			BRect				FillRoundRect(const BRect& r, float xRadius,
									float yRadius) const;

								// ellipses
			BRect				DrawEllipse(BPoint center, float xRadius,
									float yRadius, bool filled) const;

								// arcs
			BRect				StrokeArc(BPoint center, float xRadius,
									float yRadius, float angle,
									float span) const;

			BRect				FillArc(BPoint center, float xRadius,
									float yRadius, float angle,
									float span) const;

								// strings
			BRect				DrawString(const char* utf8String,
									uint32 length, BPoint baseLine,
									const escapement_delta* delta = NULL,
									BRect constrainRect = BRect(0, 0, -1, -1));

			BRect				BoundingBox(const char* utf8String,
									uint32 length, BPoint baseLine,
									const escapement_delta* delta = NULL) const;

			float				StringWidth(const char* utf8String);
			float				StringWidth(const char* utf8String,
									uint32 length);


								// bitmaps
			BRect				DrawBitmap(const RenderingBuffer* bitmap,
									BRect bitmapRect, BRect viewRect) const;

	// access to some Painter internals
			scanline_unpacked_type&
								Scanline()
									{ return *fUnpackedScanline; }
			renderer_type_ycc&	RendererYCC()
									{ return *fRendererYCC; }
			font_renderer_bin_type_ycc&	
								FontRendererYCC()
									{ return *fFontRendererYCCBin; }
			renderer_type&		RendererRGB()
									{ return *fRenderer; }
			font_renderer_bin_type&	
								FontRendererRGB()
									{ return *fFontRendererBin; }
			rasterizer_type&	Rasterizer()
									{ return *fRasterizer; }
 private:
			void				_MakeEmpty();

			void				_FilterCoord(BPoint* point,
									bool centerOffset = true) const;
			BPoint				_FilterCoord(const BPoint& point,
									bool centerOffset = true) const;

			BRect				_Clipped(const BRect& rect) const;

			void				_UpdateFont();
			void				_UpdateDrawingMode();
			void				_SetRendererColor(const rgb_color& color) const;

								// drawing functions stroke/fill
			void				_DrawBitmap(agg::rendering_buffer& srcBuffer,
									pixel_format format, BRect actualBitmapRect,
									BRect bitmapRect, BRect viewRect) const;

			void				_DrawBitmapToRGB32(
									agg::rendering_buffer& srcBuffer,
									pixel_format format, double xOffset,
									double yOffset, double xScale,
									double yScale, BRect viewRect) const;

			void				_DrawBitmapToYCbCr(
									agg::rendering_buffer& srcBuffer,
									pixel_format format, double xOffset,
									double yOffset, double xScale,
									double yScale, BRect viewRect) const;

			template <class F>
			void				_DrawBitmapNoScale(
									F copyRowFunction,
									uint32 bytesPerSourcePixel,
									uint32 bytesPerDestPixel,
									agg::rendering_buffer& srcBuffer,
									int32 xOffset, int32 yOffset,
									BRect viewRect) const;
			void				_DrawBitmapGeneric32(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const;
			void				_DrawBitmapGenericYCbCr422(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const;
			void				_DrawBitmapGenericYCbCr444(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const;
			void				_DrawBitmapGenericYCbCrA(
									agg::rendering_buffer& srcBuffer,
									double xOffset, double yOffset,
									double xScale, double yScale,
									BRect viewRect) const;

			template<class VertexSource>
			BRect				_BoundingBox(VertexSource& path) const;

			template<class VertexSource>
			BRect				_StrokePath(VertexSource& path) const;
			template<class VertexSource>
			BRect				_FillPath(VertexSource& path) const;

			void				_Render() const;

	// AGG rendering and rasterization classes
	agg::rendering_buffer*		fBuffer;
	agg::rendering_buffer*		fTempBuffer;
	BRect						fBounds;
	pixel_format				fColorSpace;
		// the buffer in memory to which
		// the Painter is "attached"

	pixfmt*						fPixelFormat;
	renderer_base*				fBaseRenderer;
	pixfmt_ycc*					fPixelFormatYCC;
	renderer_base_ycc*			fBaseRendererYCC;
		// classes with knowledge on how to
		// write color information into the buffer

	pixfmt_pre*					fPixelFormatPremultiplied;
	renderer_base_pre*			fBaseRendererPremultiplied;
	pixfmt_pre_ycc*				fPixelFormatYCCPremultiplied;
	renderer_base_pre_ycc*		fBaseRendererYCCPremultiplied;
		// classes with knowledge on how to
		// write color information into the buffer,
		// assuming the alpha channel is already encoded
		// in the color channels of each pixel

	scanline_packed_type*		fPackedScanline;
	scanline_unpacked_type*		fUnpackedScanline;
		// scanline of gemetry coverage values
		// produced by the rasterizer
	rasterizer_type*			fRasterizer;
	renderer_type*				fRenderer;
	renderer_type_ycc*			fRendererYCC;

	font_renderer_bin_type*		fFontRendererBin;
	font_renderer_bin_type_ycc*	fFontRendererYCCBin;


	struct State {
		State();
		State(State* previous);

		AffineTransform			fTransform;
		rgb_color				fColor;
		uint8					fGlobalAlpha;
		uint8					fAlpha;
		// for internal coordinate rounding/transformation
		bool					fSubpixelPrecise;
		float					fPenSize;
		cap_mode				fLineCapMode;
		join_mode				fLineJoinMode;
		float					fMiterLimit;

		State*					fPrevious;
	};

	State*						fState;
	int32						fDepth;

	int32						fComplexShapeDepth;

	// a class handling rendering and caching of glyphs -
	// it is setup to load from a specific Freetype supported
	// font file
	TextRenderer*				fTextRenderer;
};

// SetColor
inline void
Painter::SetColor(uint8 r, uint8 g, uint8 b, uint8 a)
{
	rgb_color color;
	color.red = r;
	color.green = g;
	color.blue = b;
	color.alpha = a;
	SetColor(color);
}


#endif // PAINTER_H



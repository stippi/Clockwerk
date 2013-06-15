/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Bitmap.h>
#include <Entry.h>
#include <View.h>
#include <Window.h>
#include <Screen.h>
#include <String.h>

#include <agg_basics.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_scanline_u.h>
#include <agg_scanline_p.h>
#include <agg_renderer_scanline.h>

#include <agg_ellipse.h>
#include <agg_rounded_rect.h>
#include <agg_path_storage.h>
#include <agg_conv_stroke.h>
#include <agg_conv_transform.h>

#include <agg_image_accessors.h>
#include <agg_renderer_scanline.h>
#include <agg_span_allocator.h>
#include <agg_span_interpolator_linear.h>

#include "agg_pixfmt_ycbcr422.h"
#include "agg_pixfmt_ycbcr444.h"
#include "agg_span_image_filter_ycbcr.h"

//#include "FontManager.h"

#include "VideoClip.h"
#include "VideoPreviewStream.h"

#define SPEED 2.0
#define YCBCR444 0

void
clear_bitmap(BBitmap* bitmap, BRect area)
{
	// clip area to bitmap bounds
	BRect bitmapBounds = bitmap->Bounds();
	area = area & bitmapBounds;
	// handle bitmap bounds not located at B_ORIGIN
	area.OffsetBy(-bitmapBounds.left, -bitmapBounds.top);

	uint32 width = area.IntegerWidth() + 1;
	uint32 height = area.IntegerHeight() + 1;

	uint8* bits = (uint8*)bitmap->Bits();
	uint32 bpr = bitmap->BytesPerRow();

	if (bitmap->ColorSpace() == B_YCbCr422) {
		// offset bits to left top pixel in area
		bits += (int32)area.top * bpr + (int32)area.left * 2;
	
		while (height--) {
			uint8* handle = bits;
		    for (uint32 x = 0; x < width; x++) {
				handle[0] = 16;
				handle[1] = 128;
				handle += 2;
			}
			// next row
			bits += bpr;
		}
	} else if (bitmap->ColorSpace() == B_YCbCr444) {
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
	}
}


// #pragma mark -

BBitmap* gOverlayBitmap[2];


class OverlayView : public BView {
 public:
								OverlayView(BRect frame,
									VideoPreviewStream* clip);
	virtual						~OverlayView();

	virtual	void				AttachedToWindow();
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

			void				StopVideo();

 private:
	static	int32				_frame_generator(void* cookie);
			void				_GenerateFrame(BBitmap* bitmap);
			void				_RenderVideo(BBitmap* bitmap);
			bool				_SetOverlay(BBitmap* bitmap);

			BBitmap*			fScratchBitmap;
			BRect				fDirty;

			float				fRadius;
			float				fDirection;

			thread_id			fThread;
	volatile bool				fQuitting;
	volatile bool				fPaused;

			VideoPreviewStream*	fClip;

			bigtime_t			fGenerateTime;
			bigtime_t			fShapeTime;
			bigtime_t			fDecodeTime;
			bigtime_t			fRenderTime;
			bigtime_t			fCopyTime;
			uint64				fFrameCount;
};


OverlayView::OverlayView(BRect frame, VideoPreviewStream* clip)
	: BView(frame, "overlay view", B_FOLLOW_ALL, 0),

	  fDirty(frame),

	  fRadius(50.0),
	  fDirection(SPEED),

	  fThread(-1),
	  fQuitting(false),
	  fPaused(false),

	  fClip(clip),

	  fGenerateTime(0),
	  fShapeTime(0),
	  fDecodeTime(0),
	  fRenderTime(0),
	  fCopyTime(0),
	  fFrameCount(0)
{
	gOverlayBitmap[0] = NULL;
	gOverlayBitmap[1] = NULL;
	fScratchBitmap = NULL;
}


OverlayView::~OverlayView()
{
	delete fScratchBitmap;

	delete fClip;

	if (fFrameCount == 0)
		return;

	printf("average time to generate: %lld\n", fGenerateTime / fFrameCount);
	printf("         shape rendering: %lld\n", fShapeTime / fFrameCount);
	printf("          video decoding: %lld\n", fDecodeTime / fFrameCount);
	printf("         video rendering: %lld\n", fRenderTime / fFrameCount);
	printf("     scratch buffer copy: %lld\n", fCopyTime / fFrameCount);
}


void
OverlayView::AttachedToWindow()
{
	gOverlayBitmap[0] = new BBitmap(Bounds(),
									B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL,
									B_YCbCr422);

	status_t ret = gOverlayBitmap[0]->InitCheck();
	if (ret < B_OK) {
		fprintf(stderr, "failed creating overlay: %s\n", strerror(ret));
		return;
	}

	gOverlayBitmap[1] = new BBitmap(Bounds(),
									B_BITMAP_WILL_OVERLAY,
									B_YCbCr422);

	// bitmap in main memory
#if YCBCR444
	fScratchBitmap = new BBitmap(Bounds(), 0, B_YCbCr444);
#else
	fScratchBitmap = new BBitmap(Bounds(), 0, B_YCbCr422);
#endif

	// clear bitmaps
	clear_bitmap(gOverlayBitmap[0], gOverlayBitmap[0]->Bounds());
	clear_bitmap(fScratchBitmap, fScratchBitmap->Bounds());

	// set initial overlay
	rgb_color key;
	SetViewOverlay(gOverlayBitmap[0], gOverlayBitmap[0]->Bounds(),
                   Bounds(), &key, B_FOLLOW_ALL,
                   	B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL);
	SetViewColor(key);

	// create video clip
//	entry_ref ref;
//	get_ref_for_path("/boot/home/shared/EasterBunniesJump_DivX.avi", &ref);
//	fClip = new VideoClip();
//	if (fClip->Init(&ref) < B_OK) {
//		delete fClip;
//		fClip = NULL;
//	}

	// span frame generator thread
	fThread = spawn_thread(_frame_generator, "frame generator", B_DISPLAY_PRIORITY, this);
	if (fThread >= B_OK)
		resume_thread(fThread);

	MakeFocus(true);
}


void
OverlayView::KeyDown(const char* bytes, int32 numBytes)
{
	if (bytes[0] == B_SPACE)
		fPaused = !fPaused;
}


void
OverlayView::StopVideo()
{
	if (fThread >= 0) {
		fQuitting = true;
		status_t ret;
		wait_for_thread(fThread, &ret);
	}
	ClearViewOverlay();
}


int32
OverlayView::_frame_generator(void* cookie)
{
	OverlayView* view = (OverlayView*)cookie;

	bigtime_t timeForNextFrame = system_time();
	int32 currentBitmap = 0;

	uint32 scratchBPR = view->fScratchBitmap->BytesPerRow();
	uint32 overlayBPR = gOverlayBitmap[0]->BytesPerRow();
	uint32 rows = view->fScratchBitmap->Bounds().IntegerHeight() + 1;
	uint8* scratchBits = (uint8*)view->fScratchBitmap->Bits();

	while (!view->fQuitting) {
		if (view->fPaused) {
			snooze_until(timeForNextFrame, B_SYSTEM_TIMEBASE);
		} else {
			// wait until next frame time
			snooze_until(timeForNextFrame, B_SYSTEM_TIMEBASE);
			// flip buffers
			if (!view->_SetOverlay(gOverlayBitmap[currentBitmap]))
				break;

			if (currentBitmap == 1)
				currentBitmap = 0;
			else
				currentBitmap = 1;

//printf("time since wakeup: %lld\n", system_time() - timeForNextFrame);
			// generate next frame
			view->_GenerateFrame(view->fScratchBitmap);
			bigtime_t copyTime = system_time();
			view->fShapeTime += copyTime - timeForNextFrame;
			// copy scratch buffer from main memory to gfx memory
			uint8* overlayRow = (uint8*)gOverlayBitmap[currentBitmap]->Bits();
			uint8* scratchRow = scratchBits;
#if YCBCR444
			uint32 width = view->fScratchBitmap->Bounds().IntegerWidth() + 1;
			for (uint32 y = 0; y < rows; y++) {
				uint8* dst = overlayRow;
				uint8* src = scratchRow;
				uint32 pixels = width;
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
					pixels -= 1;
				}
				overlayRow += overlayBPR;
				scratchRow += scratchBPR;
			}
#else
			for (uint32 y = 0; y < rows; y++) {
				memcpy(overlayRow, scratchRow, scratchBPR);
				overlayRow += overlayBPR;
				scratchRow += scratchBPR;
			}
#endif
			view->fCopyTime += system_time() - copyTime;
			view->fFrameCount++;
		}

		bigtime_t now = system_time();
		bigtime_t timeForRendering = now - timeForNextFrame;
		view->fGenerateTime += timeForRendering;
#if 1
		if (view->fFrameCount % 3 == 0)
			timeForNextFrame += 33334;
		else
			timeForNextFrame += 33333;
#else
		timeForNextFrame += 40000;
#endif
	}

	return 0;
}

#if YCBCR444
typedef agg::pixfmt_ycbcr444			pixfmt;
typedef agg::pixfmt_ycbcr444_pre		pixfmt_pre;
#else
typedef agg::pixfmt_ycbcr422			pixfmt;
typedef agg::pixfmt_ycbcr422_pre		pixfmt_pre;
#endif
typedef agg::renderer_base<pixfmt>		ren_base;
typedef agg::renderer_base<pixfmt_pre>	ren_base_pre;

void
OverlayView::_GenerateFrame(BBitmap* bitmap)
{
	clear_bitmap(bitmap, fDirty);

	// render video frame
	_RenderVideo(bitmap);

	// draw a circle
	agg::rendering_buffer buffer;
	buffer.attach((uint8*)bitmap->Bits(),
				  bitmap->Bounds().IntegerWidth() + 1,
				  bitmap->Bounds().IntegerHeight() + 1,
				  bitmap->BytesPerRow());

	pixfmt pixf(buffer);
	ren_base ren(pixf);
	agg::scanline_p8 sl;

	// ellipse
	if (fRadius > 250.0)
		fDirection = -SPEED;
	if (fRadius < 50.0)
		fDirection = SPEED;
	fRadius += fDirection;

	BPoint center(bitmap->Bounds().Width() / 2.0,
				  bitmap->Bounds().Height() / 2.0);
	agg::ellipse e(center.x, center.y, fRadius, fRadius, 256);
	agg::conv_stroke<agg::ellipse> s(e);
	s.width(fRadius / 2.0);

	// rasterizer
	agg::rasterizer_scanline_aa<> ras;
	ras.add_path(s);

	agg::render_scanlines_aa_solid(ras, sl, ren, agg::rgba(0.5, 1.0, 0.7));

	ras.reset();
	BRect roundRect(150.0, 50.0, 400.0, 300.0);
	agg::rounded_rect r(roundRect.left, roundRect.top,
						roundRect.right, roundRect.bottom, 40.0);
	ras.add_path(r);

	agg::render_scanlines_aa_solid(ras, sl, ren, agg::rgba(0.9, 0.01, 0.5, 0.6));

	// remember dirty area
	float inset = fRadius + s.width();
	fDirty.Set(center.x - inset, center.y - inset, center.x + inset, center.y + inset);
	fDirty = fDirty | roundRect;
}


void
OverlayView::_RenderVideo(BBitmap* bitmap)
{
	if (!fClip)
		return;

	bigtime_t decodeTime = system_time();
	const BBitmap* video = fClip->ReadFrame();
	if (!video)
		return;

	bigtime_t renderTime = system_time();

	uint32 mode = 0;

	if (mode == 0) {
		// copy video bitmap into provided bitmap
		uint8* bitmapRow = (uint8*)bitmap->Bits();
		uint8* videoRow = (uint8*)video->Bits();
		uint32 rows = min_c(video->Bounds().IntegerHeight() + 1,
							bitmap->Bounds().IntegerHeight() + 1);
		uint32 bitmapBPR = bitmap->BytesPerRow();
		uint32 videoBPR = video->BytesPerRow();
		uint32 bytesToCopy = min_c(bitmapBPR, videoBPR);
		if (bitmap->ColorSpace() == video->ColorSpace()) {
			// same color space
			for (uint32 y = 0; y < rows; y++) {
				memcpy(bitmapRow, videoRow, bytesToCopy);
				bitmapRow += bitmapBPR;
				videoRow += videoBPR;
			}
		} else {
			uint32 width = min_c(video->Bounds().IntegerWidth() + 1,
								 bitmap->Bounds().IntegerWidth() + 1);
			if (video->ColorSpace() == B_YCbCr422) {
				// YCbCr422 -> YCbCr444
				for (uint32 y = 0; y < rows; y++) {
					uint8* dst = bitmapRow;
					uint8* src = videoRow;
					for (uint32 x = 0; x < width / 2; x++) {
						dst[0] = src[0];
						dst[1] = src[1];
						dst[2] = src[3];

						dst[3] = src[2];
						dst[4] = src[1];
						dst[5] = src[3];
						dst += 6;
						src += 4;
					}
					bitmapRow += bitmapBPR;
					videoRow += videoBPR;
				}
			} else {
				// YCbCr444 -> YCbCr422
				for (uint32 y = 0; y < rows; y++) {
					uint8* dst = bitmapRow;
					uint8* src = videoRow;
					for (uint32 x = 0; x < width / 2; x++) {
						dst[0] = src[0];
						dst[1] = (src[1] + src[4]) >> 1;
						dst[2] = src[3];
						dst[3] = (src[2] + src[5]) >> 1;
						dst += 4;
						src += 6;
					}
					bitmapRow += bitmapBPR;
					videoRow += videoBPR;
				}
			}
		}
	}

	if (mode == 1) {
		// scaled copy of video into provided bitmap
		float xScale = 1.5;
		float yScale = 1.5;

		uint8* dst = (uint8*)bitmap->Bits();
		uint8* src = (uint8*)video->Bits();

		uint32 width = video->Bounds().IntegerWidth() + 1;
		uint32 height = video->Bounds().IntegerHeight() + 1;

		uint32 sWidth = (uint32)floorf(width * xScale + 0.5);
		uint32 sHeight = (uint32)floorf(height * yScale + 0.5);

		uint32 srcBPR = video->BytesPerRow();
		uint32 dstBPR = bitmap->BytesPerRow();

		// calculate access pattern and write into LUT
		uint32 yAccess[sWidth];
		uint32 cAccess[sWidth];

		uint32 cols = min_c(sWidth, (uint32)bitmap->Bounds().IntegerWidth() + 1);
		uint32 rows = min_c(sHeight, (uint32)bitmap->Bounds().IntegerHeight() + 1);

#if YCBCR444
		uint32 sw = sWidth / 2;
		uint32 xi = 0;
		for (uint32 x = 0; x < sw; x++) {
			uint32 index = uint32(float(x * width) / (float)sWidth + 0.5) * 2;
			cAccess[xi++] = index * 2 + 1;
			cAccess[xi++] = index * 2 + 3;
		}

		for (uint32 x = 0; x < sWidth; x++) {
			uint32 index = uint32(float(x * width) / (float)sWidth + 0.5);
			yAccess[x] = index * 2;
		}

		for (uint32 y = 0; y < rows; y++) {
			uint32 row = (y * height) / sHeight;
			uint8* srcHandle = src + row * srcBPR;
			uint8* dstHandle = dst;
			for (uint32 x = 0; x < cols; x++) {
				*dstHandle++ = srcHandle[yAccess[x]];
				*dstHandle++ = srcHandle[cAccess[x]];
				*dstHandle++ = srcHandle[cAccess[x]];
			}
			dst += dstBPR;
		}
#else
		uint32 sw = sWidth / 2;
		uint32 xi = 0;
		for (uint32 x = 0; x < sw; x++) {
			uint32 index = uint32(float(x * width) / (float)sWidth + 0.5) * 2;
			cAccess[xi++] = index * 2 + 1;
			cAccess[xi++] = index * 2 + 3;
		}

		for (uint32 x = 0; x < sWidth; x++) {
			uint32 index = uint32(float(x * width) / (float)sWidth + 0.5);
			yAccess[x] = index * 2;
		}

		for (uint32 y = 0; y < rows; y++) {
			uint32 row = (y * height) / sHeight;
			uint8* srcHandle = src + row * srcBPR;
			uint8* dstHandle = dst;
			for (uint32 x = 0; x < cols; x++) {
				*dstHandle++ = srcHandle[yAccess[x]];
				*dstHandle++ = srcHandle[cAccess[x]];
			}
			dst += dstBPR;
		}
#endif
	}

	if (mode == 2) {
	
		// AGG pipeline
	
		// rendering buffer attached to target bitmap
		agg::rendering_buffer targetBuffer;
		targetBuffer.attach((uint8*)bitmap->Bits(),
							bitmap->Bounds().IntegerWidth() + 1,
							bitmap->Bounds().IntegerHeight() + 1,
							bitmap->BytesPerRow());
		// pixel format attached to target rendering buffer
		pixfmt_pre pixf_pre(targetBuffer);
		ren_base_pre rb_pre(pixf_pre);
	
	
		// rendering buffer attached to video bitmap
		agg::rendering_buffer buffer;
		buffer.attach((uint8*)video->Bits(),
					  video->Bounds().IntegerWidth() + 1,
					  video->Bounds().IntegerHeight() + 1,
					  video->BytesPerRow());
		// pixel format attached to video rendering buffer
		pixfmt pixf_img(buffer);
	
		// image transformation matrix
		agg::trans_affine srcMatrix;
//		srcMatrix *= agg::trans_affine_rotation(agg::deg2rad(2.0));
		srcMatrix *= agg::trans_affine_scaling(1.2, 0.8);
//		srcMatrix *= agg::trans_affine_translation(20.0, 20.0);
	//	srcMatrix *= fTransform;
	
		agg::trans_affine imgMatrix = srcMatrix;
		imgMatrix.invert();
	
		// image interpolator
		typedef agg::span_interpolator_linear<> interpolator_type;
		interpolator_type interpolator(imgMatrix);
	
		// image accessor
		typedef agg::image_accessor_clip<pixfmt> source_type;
		source_type source(pixf_img, agg::ycbcra8(0, 0, 0, 0));
	
		// scanline, rasterizer, allocator
		agg::rasterizer_scanline_aa<> ras;
		agg::scanline_u8 sl;
		agg::span_allocator<pixfmt::color_type> sa;
	
		// path enclosing video bitmap
		BRect vb = video->Bounds();
		agg::path_storage path;
		path.move_to(vb.left, vb.top);
		path.line_to(vb.right + 1, vb.top);
		path.line_to(vb.right + 1, vb.bottom + 1);
		path.line_to(vb.left, vb.bottom + 1);
		path.close_polygon();
	
		// add transformed path to rasterizer
		agg::conv_transform<agg::path_storage> transformedPath(path, srcMatrix);
		ras.add_path(transformedPath);
	
		// render
		if (true) {
			// nearest neighbor filter
			typedef agg::span_image_filter_ycbcr422_nn<source_type, interpolator_type> span_gen_type;
			span_gen_type sg(source, interpolator);
			agg::render_scanlines_aa(ras, sl, rb_pre, sa, sg);
	//	} else {
	//		// bilinear filter
	//		typedef agg::span_image_filter_ycbcr444_bilinear<pixfmt, interpolator_type> span_gen_type;
	//		span_gen_type sg(pixf_img, agg::rgba_pre(0,0,0,0), interpolator);
	//		agg::render_scanlines_aa(ras, sl, rb_pre, sa, sg);
		}
	}

	bigtime_t finishTime = system_time();
	fDecodeTime += renderTime - decodeTime;
	fRenderTime += finishTime - renderTime;
}


bool
OverlayView::_SetOverlay(BBitmap* bitmap)
{
	if (fQuitting)
		return false;

	if (!LockLooper())
		return false;

	rgb_color key;
	SetViewOverlay(bitmap,
				   bitmap->Bounds(),
				   Bounds(),
				   &key, B_FOLLOW_ALL,
				   B_OVERLAY_FILTER_HORIZONTAL | B_OVERLAY_FILTER_VERTICAL
				   | B_OVERLAY_TRANSFER_CHANNEL);
//	SetViewColor(key);

	UnlockLooper();

	return true;
}


// #pragma mark -


class Window : public BWindow {
 public:
								Window(BRect frame, VideoPreviewStream* clip);
	virtual						~Window();

	virtual	bool				QuitRequested();

 private:
			OverlayView*		fView;
};


Window::Window(BRect frame, VideoPreviewStream* clip)
	: BWindow(frame, "Overlay Test", B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	frame.OffsetTo(B_ORIGIN);
	fView = new OverlayView(frame, clip);
	AddChild(fView);
}


Window::~Window()
{
}


bool
Window::QuitRequested()
{
	fView->StopVideo();
	
	snooze(10000);
	delete gOverlayBitmap[0];
	snooze(10000);
	delete gOverlayBitmap[1];

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


// #pragma mark -


int
main(int argc, char** argv)
{
//	FontManager::CreateDefault();

	// dummy BApplication
	BApplication app("application/x-vnd.Mindwork-Overlay");

	BString file("/boot/home/media/data/9live.avi");
	entry_ref ref;
	get_ref_for_path(file.String(), &ref);
	VideoClip* clip = new VideoClip();
	if (clip->Init(&ref) < B_OK) {
		delete clip;
		return -1;
	}
	VideoPreviewStream::CreateStream(file, clip);
	delete clip;

	VideoPreviewStream* stream = new VideoPreviewStream();
	status_t ret = stream->Init(file);
	if (ret < B_OK)
		printf("preview stream init: %s\n", strerror(ret));

	BRect frame(0.0, 0.0, 852.0, 479.0);
//	BRect frame(0.0, 0.0, 1023.0, 575.0);
// NOTE: careful: at this high resolution, the test runs on my box, but app_server
// crashes when the test quits, maybe there is another bug though that causes this
//	BRect frame(0.0, 0.0, 1365.0, 767.0);
	frame.OffsetBy(20.0, 50.0);
	BWindow* window = new Window(frame, stream);

	// go
	window->Show();
	app.Run();

//	FontManager::DeleteDefault();

	return 0;
}

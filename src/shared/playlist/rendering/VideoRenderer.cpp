/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "VideoRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Entry.h>
#include <MediaFile.h>

#include "common.h"
#include "support.h"

#include "AudioTrackReader.h"
#include "MediaClip.h"
#include "MediaRenderingBuffer.h"
#include "MemoryBuffer.h"
#include "Painter.h"
#include "ToneProducerReader.h"

using std::nothrow;

#define DEBUG_DECODED_FRAME 0
#if DEBUG_DECODED_FRAME
#  include <Bitmap.h>
#  include <BitmapStream.h>
#  include <File.h>
#  include <TranslatorRoster.h>
#endif // DEBUG_DECODED_FRAME

// constructor
VideoRenderer::VideoRenderer(ClipPlaylistItem* item,
							 MediaClip* clip, color_space format)
	:
	ClipRenderer(item, clip),
	fMediaFile(NULL),
	fVideoTrack(NULL),

	fDisplayBounds(0, 0, -1, -1),

	fBuffer(NULL),
	fColorSpaceConversionBuffer(NULL),
	fFrameCount(0),
	fCurrentFrame(-1),

	fNoBufferErrorPrinted(false)

	#if VIDEO_DECODE_TIMING
	,
	fDecodeTime(0),
	fFramesDecoded(0)
	#endif
{
	// check if this file is a BMediaFile
	fMediaFile = new (nothrow) BMediaFile(clip->Ref());
	status_t ret = fMediaFile ? fMediaFile->InitCheck() : B_NO_MEMORY;
	if (ret < B_OK) {
		print_error("VideoRenderer() - "
					"failed to create media file: %s\n", strerror(ret));
		return;
	}

	// count the tracks and instanciate them, one at a time
	// find the first video track
	int32 numTracks = fMediaFile->CountTracks();
	for (int32 i = 0; i < numTracks; i++) {
		if (fVideoTrack)
			break;
		BMediaTrack* track = fMediaFile->TrackAt(i);
		if (!track) {
			print_error("VideoRenderer() - cannot read media track at %ld\n", i);
			continue;
		}
		// get the encoded format
		media_format encodedFormat;
		ret = track->EncodedFormat(&encodedFormat);
		if (ret < B_OK) {
			print_error("VideoRenderer() - cannot understand encoded format "
						"of track %ld: %s\n", i, strerror(ret));
			fMediaFile->ReleaseTrack(track);
			continue;
		}
		bool release = true;
		switch(encodedFormat.type) {
		 	case B_MEDIA_RAW_VIDEO:
		 	case B_MEDIA_ENCODED_VIDEO: {
				// video track
				if (!fVideoTrack) {
					fVideoTrack = track;
					release = false;
				}
				break;
		 	}
			case B_MEDIA_RAW_AUDIO:
			case B_MEDIA_ENCODED_AUDIO:
			default:
				break;
		}
		if (release) {
			// didn't do anything with the track
			fMediaFile->ReleaseTrack(track);
		}
	}

	if (!fVideoTrack) {
		print_error("VideoRenderer() - failed to find a video track\n");
		return;
	}

	// get the encoded format
	memset(&fFormat, 0, sizeof(media_format));
	ret = fVideoTrack->EncodedFormat(&fFormat);
	if (ret < B_OK) {
		print_error("VideoRenderer::InitCheck() - "
					"fVideoTrack->EncodedFormat(): %s\n", strerror(ret));
		return;
	}

	// get ouput video frame size
	uint32 width = fFormat.u.encoded_video.output.display.line_width;
	uint32 height = fFormat.u.encoded_video.output.display.line_count;

	// NOTE: B_YCbCr444 is not directly supported, Painter knows
	// how to render both, if the format is either 422 or 444
	if (format == B_YCbCr444 || format == B_YCbCr422)
		format = B_YCbCr422;
	else
		format = B_RGB32;

	// specifiy the decoded format. we derive this information from
	// the encoded format (width & height).
	memset(&fFormat, 0, sizeof(media_format));
	fFormat.u.raw_video.last_active = height - 1;
//	fFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
	fFormat.u.raw_video.display.format = format;
	fFormat.u.raw_video.display.line_width = width;
	fFormat.u.raw_video.display.line_count = height;
	if (format == B_RGB32 || format == B_RGBA32)
		fFormat.u.raw_video.display.bytes_per_row = width * 4;
	else if (format == B_YCbCr422)
		fFormat.u.raw_video.display.bytes_per_row = ((width * 2 + 3) / 4) * 4;

	ret = fVideoTrack->DecodedFormat(&fFormat);

	if (ret < B_OK) {
		print_error("VideoRenderer() - "
					"fVideoTrack->DecodedFormat(): %s\n", strerror(ret));
		return;
	}

	if (fFormat.u.raw_video.display.format != format) {
		print_error("### File %s - codec changed colorspace of decoded format!\n"
					"    this is bad for performance, since colorspace conversion\n"
					"    needs to happen during playback.\n",
					clip->Ref()->name);
		uint32 minBPR;
		format = fFormat.u.raw_video.display.format;
		if (format == B_YCbCr422)
			minBPR = ((width * 2 + 3) / 4) * 4;
		else
			minBPR = width * 4;
		if (minBPR > fFormat.u.raw_video.display.bytes_per_row) {
			print_error("  -> stupid codec forgot to adjust bytes per row!\n");
			fFormat.u.raw_video.display.bytes_per_row = minBPR;
		}
	}

	// allocate a buffer large enough to contain the decoded frame.
	uint32 bufferSize = height * fFormat.u.raw_video.display.bytes_per_row;
	fBuffer = new (nothrow) uint8[bufferSize];

	if (!fBuffer) {
		print_error("VideoRenderer::InitCheck() - "
					"no memory for decoded video frame\n");
		return;
	}

	fFrameCount = fVideoTrack->CountFrames();
	fDisplayBounds = MediaClip::VideoBounds(fFormat);

//	int64 lastKeyFrame = 0;
//	for (uint64 frame = 0; frame < fFrameCount; frame++) {
//		int64 keyFrame = frame;
//		fVideoTrack->FindKeyFrameForFrame(&keyFrame,
//			B_MEDIA_SEEK_CLOSEST_BACKWARD);
//		if (lastKeyFrame != keyFrame) {
//			printf("%lld...%lld -> %lld\n", lastKeyFrame, keyFrame - 1, lastKeyFrame);
//			lastKeyFrame = keyFrame;
//		}
//	}
//	printf("%lld...%lld -> %lld\n", lastKeyFrame, fFrameCount - 1, lastKeyFrame);
}

// destructor
VideoRenderer::~VideoRenderer()
{
	#if VIDEO_DECODE_TIMING
	if (fFramesDecoded > 0) {
		printf("average decoding time per frame: %lld\n",
			fDecodeTime / fFramesDecoded);
	}
	#endif

	if (fMediaFile) {
		if (fVideoTrack)
			fMediaFile->ReleaseTrack(fVideoTrack);
		delete fMediaFile;
	}
	delete[] fBuffer;
	delete fColorSpaceConversionBuffer;
}

// Generate
status_t
VideoRenderer::Generate(Painter* painter, double _frame,
	const RenderPlaylistItem* item)
{
	if (!fBuffer) {
		if (!fNoBufferErrorPrinted) {
			print_error("VideoRenderer::Generate() - no buffer!\n");
			fNoBufferErrorPrinted = true;
		}
		return B_NO_INIT;
	}

	_frame = _frame * fFormat.u.raw_video.field_rate
		/ PlaylistVideoFrameRate();
	int64 frame = (int64)(_frame + 0.5);
//printf("video renderer frame: %lld\n", frame);

	// attach RenderingBuffer to raw decoding buffer
	MediaRenderingBuffer mediaBuffer(fBuffer, &fFormat);
	RenderingBuffer* renderingBuffer = &mediaBuffer;
	if (mediaBuffer.PixelFormat() != painter->PixelFormat()) {
		// this only happens if the codec didn't support the
		// painters colorspace, this should only be the case
		// if the painter is YCbCr422, because all codecs
		// would support RGB[A]32
		// TODO: currently the decoding buffer can not be changed
		// in any way during runtime, but if it could, then
		// we would have to make sure that the conversion buffer
		// still matches (width, height...)
		if (!fColorSpaceConversionBuffer) {
			uint32 width = mediaBuffer.Width();
			uint32 height = mediaBuffer.Height();
			uint32 bpr = ((width * 3 + 3) / 4) * 4;
			fColorSpaceConversionBuffer
				= new (nothrow) MemoryBuffer(width, height, YCbCr444, bpr);
		}
		renderingBuffer = fColorSpaceConversionBuffer;
	}

	if (fCurrentFrame != frame && fCurrentFrame != frame - 1) {
//printf("seek\n");
//printf("VideoRenderer::Generate() - %lld -> %lld, %.2f (%.3f)\n", fCurrentFrame, frame, _frame, fFormat.u.raw_video.field_rate);
		// seeking is necessary
		status_t ret = B_OK;
		if (fCurrentFrame == frame - 2) {
			// TODO: this could be removed, seeking works properly
			// and would handle this case as well (I keep it here,
			// because it might be just a tiny bit faster)
			// NOTE: special case to just skip one frame
			// this helps to playback 30fps movies at 25fps correctly
			int64 frameCount = 1;
			ret = fVideoTrack->ReadFrames(fBuffer, &frameCount);
		} else {
			// real seek
			int64 keyFrame = frame;
			ret = fVideoTrack->FindKeyFrameForFrame(&keyFrame,
				B_MEDIA_SEEK_CLOSEST_BACKWARD);
			if (ret == B_OK) {
				if (keyFrame > fCurrentFrame || fCurrentFrame > frame)
					ret = fVideoTrack->SeekToFrame(&keyFrame, 0);
				else
					keyFrame = fCurrentFrame;
			} else {
				// hack to so that seeking at least works when
				// seeking (close) to the beginning of a file
				// in case SeekToFrame() failed
				if (keyFrame < 3)
					keyFrame = 0;
				ret = fVideoTrack->SeekToFrame(&keyFrame,
					B_MEDIA_SEEK_CLOSEST_BACKWARD);
			}

//printf("seeked to frame: %lld (wanted: %lld/%.2f)\n", keyFrame,
//	frame, _frame);
			int64 currentFrame = keyFrame;
			#if VIDEO_DECODE_TIMING
			bigtime_t now = system_time();
			#endif
			while (currentFrame < frame && ret >= B_OK) {
				int64 frameCount = 1;
				ret = fVideoTrack->ReadFrames(fBuffer, &frameCount);
				currentFrame += frameCount;
				#if VIDEO_DECODE_TIMING
				fFramesDecoded++;
				#endif
			}
			#if VIDEO_DECODE_TIMING
			fDecodeTime += system_time() - now;
			fFramesDecoded++;
			#endif
		}
		if (ret < B_OK) {
			print_error("VideoRenderer::Generate() - "
						"error while seeking into the track: %s\n",
						strerror(ret));
			return ret;
		}
	}

	if (fCurrentFrame != frame) {
//printf("decode\n");
		// read a frame
		int64 frameCount = 1;
		// TODO: how does this work for interlaced video (field count > 1)?
		#if VIDEO_DECODE_TIMING
		bigtime_t now = system_time();
		#endif
		status_t ret = fVideoTrack->ReadFrames(fBuffer, &frameCount);
		#if VIDEO_DECODE_TIMING
		fDecodeTime += system_time() - now;
		fFramesDecoded++;
		#endif

		if (ret < B_OK) {
			if (ret != B_LAST_BUFFER_ERROR) {
				print_error("VideoRenderer::Generate() - "
							"error while reading frame of track: %s\n",
							strerror(ret));
			}
			return ret;
		}
		fCurrentFrame = frame;

		if (fColorSpaceConversionBuffer) {
			_ConvertToYCbRr(&mediaBuffer, fColorSpaceConversionBuffer);
		} else if (fFormat.u.raw_video.display.format != B_YCbCr422) {
			// post process alpha channel :-/
			// TODO: find a work arround
			uint8* b = fBuffer;
			int32 pixelCount = mediaBuffer.Width() * mediaBuffer.Height();
			for (int32 i = 0; i < pixelCount; i++) {
				b[3] = 255;
				b += 4;
			}
		}
	} // else buffer already contains the right frame

#if DEBUG_DECODED_FRAME
if (modifiers() & B_SHIFT_KEY) {
BFile fileStream("/boot/home/Desktop/decoded.png", B_READ_WRITE | B_CREATE_FILE | B_ERASE_FILE);
BTranslatorRoster* roster = BTranslatorRoster::Default();
BBitmap* bitmap = new BBitmap(mediaBuffer.Bounds(), 0, mediaBuffer.ColorSpace());
memcpy(bitmap->Bits(), fBuffer, bitmap->BitsLength());
BBitmapStream bitmapStream(bitmap);
roster->Translate(&bitmapStream, NULL, NULL, &fileStream, B_PNG_FORMAT, 0);
bitmapStream.DetachBitmap(&bitmap);
delete bitmap;
}
#endif // DEBUG_DECODED_FRAME

	painter->SetSubpixelPrecise(false);
	painter->DrawBitmap(renderingBuffer, renderingBuffer->Bounds(),
		fDisplayBounds);

	return B_OK;
}

// IsSolid
bool
VideoRenderer::IsSolid(double frame) const
{
	return true;
}

// CurrentTime
bigtime_t
VideoRenderer::CurrentTime() const
{
	return fVideoTrack->CurrentTime();
}

// CurrentFrame
bigtime_t
VideoRenderer::CurrentFrame() const
{
	return fVideoTrack->CurrentFrame();
}

// FindKeyFrameForFrame
status_t
VideoRenderer::FindKeyFrameForFrame(int64* _inOutFrame, int32 flags) const
{
	return fVideoTrack->FindKeyFrameForFrame(_inOutFrame, flags);
}

// SeekToFrame
status_t
VideoRenderer::SeekToFrame(int64* _inOutFrame, int32 flags)
{
	return fVideoTrack->SeekToFrame(_inOutFrame, flags);
}

// ReadChunk
status_t
VideoRenderer::ReadChunk(const void** _buffer, size_t* _size,
	media_header* mediaHeader)
{
	return fVideoTrack->ReadChunk((char**)_buffer, (int32*)_size,
		mediaHeader);
}

// ReadChunk
status_t
VideoRenderer::GetCodecInfo(media_codec_info* _codecInfo) const
{
	return fVideoTrack->GetCodecInfo(_codecInfo);
}

// #pragma mark -

// _ConvertToYCbRr
void
VideoRenderer::_ConvertToYCbRr(RenderingBuffer* src, RenderingBuffer* dst)
{
	if ((src->PixelFormat() != BGR32 && src->PixelFormat() != BGRA32)
		|| dst->PixelFormat() != YCbCr444)
		return;
	// convert from B_RGB32/B_RGBA32 to YCbCr444
	uint32 width = src->Width();
	uint32 height = src->Height();

	uint32 srcBPR = src->BytesPerRow();
	uint32 dstBPR = dst->BytesPerRow();

	uint8* srcBits = (uint8*)src->Bits();
	uint8* dstBits = (uint8*)dst->Bits();

	// source is expected to be in B_RGB32 color space
	for (uint32 y = 0; y < height; y++) {
		uint8* s = srcBits;
		uint8* d = dstBits;
		for (uint32 x = 0; x < width; x++) {
			d[0] = ((8432 * s[2] + 16425 * s[1] + 3176 * s[0]) >> 15) + 16;
			d[1] = ((-4818 * s[2] - 9527 * s[1] + 14345 * s[0]) >> 15) + 128;
			d[2] = ((14345 * s[2] - 12045 * s[1] - 2300 * s[0]) >> 15) + 128;
			s += 4;
			d += 3;
		}
		srcBits += srcBPR;
		dstBits += dstBPR;
	}
}


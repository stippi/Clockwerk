/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef VIDEO_RENDERER_H
#define VIDEO_RENDERER_H

#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MediaTrack.h>

#include "ClipRenderer.h"

#define VIDEO_DECODE_TIMING 0

class BMediaFile;
class MediaClip;
class MemoryBuffer;
class RenderingBuffer;

class VideoRenderer : public ClipRenderer {
public:
								VideoRenderer(ClipPlaylistItem* item,
									MediaClip* clip, color_space format);
	virtual						~VideoRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	bool				IsSolid(double frame) const;

			BRect				DisplayBounds() const
									{ return fDisplayBounds; }

	// BMediaTrack pass-thru API
			bigtime_t			CurrentTime() const;
			int64				CurrentFrame() const;

			status_t			FindKeyFrameForFrame(int64* _inOutFrame,
									int32 flags = 0) const;
			status_t			SeekToFrame(int64* _inOutFrame,
									int32 flags = 0);

			status_t			ReadChunk(const void** _buffer, size_t* _size,
									media_header* mediaHeader = NULL);

			status_t			GetCodecInfo(
									media_codec_info* _codecInfo) const;

private:
			void				_ConvertToYCbRr(RenderingBuffer* src,
									RenderingBuffer* dst);

			BMediaFile*			fMediaFile;
			BMediaTrack*		fVideoTrack;

			media_format		fFormat;
			BRect				fDisplayBounds;

			uint8*				fBuffer;
			MemoryBuffer*		fColorSpaceConversionBuffer;
			uint64				fFrameCount;
			int64				fCurrentFrame;

			bool				fNoBufferErrorPrinted;

			#if VIDEO_DECODE_TIMING
			bigtime_t			fDecodeTime;
			int64				fFramesDecoded;
			#endif
};

#endif // VIDEO_RENDERER_H

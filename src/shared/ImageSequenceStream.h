/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef IMAGE_SEQUENCE_STREAM_H
#define IMAGE_SEQUENCE_STREAM_H

#include "RenderingBuffer.h"

class BBitmap;
class BFile;
class VideoClip;

class ImageSequenceStream {
 public:
								ImageSequenceStream();
	virtual						~ImageSequenceStream();

			status_t			Init(const BString& id);
			status_t			Init(uint32 width, uint32 height,
									pixel_format format, int64 frameCount,
									const BString& id);

			status_t			GetFormat(uint32* width,
									uint32* height,
									pixel_format* format,
									uint32* bytesPerRow) const;

			status_t			SeekToFrame(int64 frame);
			int64				CurrentFrame() const
									{ return fCurrentFrame; }

			status_t			ReadFrame(uint8* buffer);
			const BBitmap*		ReadFrame();

			status_t			WriteFrame(uint8* buffer);
			status_t			Finalize();

	static	status_t			CreateStream(const BString& id,
									VideoClip* renderer);

 private:
			struct stream_header {
								stream_header();
								stream_header(uint32 width, uint32 height,
									pixel_format format);
				uint32			width;
				uint32			height;
				pixel_format	format;
				int64			frameCount;
			};

			void				_MakeEmpty();
	static	status_t			_GetRefForID(const BString& serverID,
									entry_ref* ref);
	static	status_t			_EncodeBitmap(const BBitmap* bitmap,
									BFile& file);
			status_t			_DecodeBitmap(const BBitmap* bitmap,
									BFile& file);
			status_t			_DecodeBuffer(uint8* buffer,
									uint32 width, uint32 height, uint32 bpr,
									BFile& file);
			void				_CheckStreamPosition();

			stream_header		fHeader;
			int64				fCurrentFrame;

			BFile*				fStream;

			off_t*				fFrameOffsetMap;

			BBitmap*			fDecodedBitmap;
};

#endif // IMAGE_SEQUENCE_STREAM_H

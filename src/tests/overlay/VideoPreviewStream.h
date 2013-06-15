/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef VIDEO_PREVIEW_STREAM_H
#define VIDEO_PREVIEW_STREAM_H

#include <MediaDefs.h>

class BBitmap;
class BFile;
//class VideoRenderer;
class VideoClip;

class VideoPreviewStream {
 public:
								VideoPreviewStream();
	virtual						~VideoPreviewStream();

			status_t			Init(const BString& id);

			status_t			GetFormat(media_format* format);

			status_t			SeekToFrame(int64 frame);
			int64				CurrentFrame() const
									{ return fCurrentFrame; }

			status_t			ReadFrame(uint8* buffer);
			const BBitmap*		ReadFrame();

	

	static	status_t			CreateStream(const BString& id,
//									VideoRenderer* renderer);
									VideoClip* renderer);

 private:
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

			media_format		fFormat;
			int64				fCurrentFrame;
			int32				fFrameCount;

			BFile*				fStream;

			off_t*				fFrameOffsetMap;

			BBitmap*			fDecodedBitmap;
};

#endif // VIDEO_PREVIEW_STREAM_H

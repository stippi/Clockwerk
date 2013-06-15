/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef VIDEO_CLIP_H
#define VIDEO_CLIP_H

#include <MediaDefs.h>

class BBitmap;
class BMediaFile;
class BMediaTrack;

class VideoClip {
 public:
								VideoClip();
	virtual						~VideoClip();

			uint64				CountFrames() const;
			status_t			SeekToFrame(int64* frame);

			const BBitmap*		ReadFrame();

			status_t			Init(const entry_ref* ref);

			status_t			GetFormat(media_format* format) const;

 private:
			BMediaFile*			fMediaFile;
			BMediaTrack*		fTrack;
			media_format		fFormat;
			BBitmap*			fBuffer;
			uint64				fFrameCount;
			int64				fCurrentFrame;
};

#endif // VIDEO_CLIP_H

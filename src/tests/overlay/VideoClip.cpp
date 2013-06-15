/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "VideoClip.h"

#include <new>
#include <stdio.h>
#include <string.h>


#include <Bitmap.h>
#include <MediaTrack.h>
#include <MediaFile.h>

using std::nothrow;

// constructor
VideoClip::VideoClip()
	: fMediaFile(NULL),
	  fTrack(NULL),
	  fBuffer(NULL),
	  fFrameCount(0),
	  fCurrentFrame(0)
{
}

// destructor
VideoClip::~VideoClip()
{
	fMediaFile->ReleaseTrack(fTrack);
	delete fMediaFile;
	delete fBuffer;
}

// CountFrames
uint64
VideoClip::CountFrames() const
{
	return fFrameCount;
}

// SeekToFrame
status_t
VideoClip::SeekToFrame(int64* frame)
{
	if (!frame)
		return B_BAD_VALUE;

	status_t ret = fTrack->SeekToFrame(frame, B_MEDIA_SEEK_CLOSEST_BACKWARD);
	if (ret == B_OK)
		fCurrentFrame = *frame;

	return ret;
}

// ReadFrame
const BBitmap*
VideoClip::ReadFrame()
{
	if (!fBuffer) {
		fprintf(stderr, "VideoClip::Generate() - no buffer!\n");
		return NULL;
	}

	fCurrentFrame++;

	if (fCurrentFrame == (int64)fFrameCount) {
		// seeking back to beginning of clip
		fCurrentFrame = 0;
		status_t err = fTrack->SeekToFrame(&fCurrentFrame,
										   B_MEDIA_SEEK_CLOSEST_BACKWARD);
		if (err < B_OK) {
			fprintf(stderr, "VideoClip::Generate() - "
							"error while seeking into the track: %s\n",
							strerror(err));
			return NULL;
		}
	}

	// read a frame
	int64 frameCount = 1;
	status_t err = fTrack->ReadFrames(fBuffer->Bits(), &frameCount);

	if (err < B_OK) {
		fprintf(stderr, "VideoClip::Generate() - "
						"error while reading frame of track: %s\n",
						strerror(err));
		return NULL;
	}

	return fBuffer;
}

// #pragma mark -

// string_for_color_space
const char*
string_for_color_space(color_space format)
{
	const char* name = "<unkown format>";
	switch (format) {
		case B_RGB32:
			name = "B_RGB32";
			break;
		case B_RGBA32:
			name = "B_RGBA32";
			break;
		case B_RGB32_BIG:
			name = "B_RGB32_BIG";
			break;
		case B_RGBA32_BIG:
			name = "B_RGBA32_BIG";
			break;
		case B_RGB24:
			name = "B_RGB24";
			break;
		case B_RGB24_BIG:
			name = "B_RGB24_BIG";
			break;
		case B_CMAP8:
			name = "B_CMAP8";
			break;
		case B_GRAY8:
			name = "B_GRAY8";
			break;
		case B_GRAY1:
			name = "B_GRAY1";
			break;

		// YCbCr
		case B_YCbCr422:
			name = "B_YCbCr422";
			break;
		case B_YCbCr411:
			name = "B_YCbCr411";
			break;
		case B_YCbCr444:
			name = "B_YCbCr444";
			break;
		case B_YCbCr420:
			name = "B_YCbCr420";
			break;

		// YUV
		case B_YUV422:
			name = "B_YUV422";
			break;
		case B_YUV411:
			name = "B_YUV411";
			break;
		case B_YUV444:
			name = "B_YUV444";
			break;
		case B_YUV420:
			name = "B_YUV420";
			break;

		case B_YUV9:
			name = "B_YUV9";
			break;
		case B_YUV12:
			name = "B_YUV12";
			break;

		default:
			break;
	}
	return name;
}

// Init
status_t
VideoClip::Init(const entry_ref* ref)
{
	// check if this file is a MediaFile
	// instantiate a BMediaFile object, and make sure there was no error.
	fMediaFile = new BMediaFile(ref);
	status_t err = fMediaFile->InitCheck();
	if (err < B_OK) {
		fprintf(stderr, "VideoClip::Init() - "
						"no media file: %s\n", strerror(err));
		return err;
	}
	// count the tracks and instanciate them, one at a time
	int32 numTracks = fMediaFile->CountTracks();
	for (int32 i = 0; i < numTracks; i++) {
		if (fTrack)
			break;

		BMediaTrack* track = fMediaFile->TrackAt(i);
		if (!track) {
			fprintf(stderr, "VideoClip::Init() - "
							"cannot read media track at %ld\n", i);
			continue;
		}
		// get the encoded format
		media_format format;
		err = track->EncodedFormat(&format);
		if (err < B_OK) {
			fprintf(stderr, "VideoClip::Init() - "
							"cannot understand encoded format "
							"of track %ld: %s\n", i, strerror(err));
			continue;
		}
		switch(format.type) {
		 	case B_MEDIA_RAW_VIDEO:
		 	case B_MEDIA_ENCODED_VIDEO: {
				// video track
				fTrack = track;
				break;
		 	}
			case B_MEDIA_RAW_AUDIO:
				// audio track
				break;
			case B_MEDIA_ENCODED_AUDIO:
				// audio track
				break;
			default:
				break;
		}
		if (fTrack != track) {
			// didn't do anything with the track
			fMediaFile->ReleaseTrack(track);
		}
	}

	if (!fTrack)
		return B_UNSUPPORTED;

	err = fTrack->EncodedFormat(&fFormat);
	if (err < B_OK) {
		fprintf(stderr, "VideoClip::Init() - "
						"fTrack->EncodedFormat(): %s\n",
						strerror(err));
		return err;
	}

	// get encoded video frame size
	uint32 width = fFormat.u.encoded_video.output.display.line_width;
	uint32 height = fFormat.u.encoded_video.output.display.line_count;
printf("VideoClip::Init() - native colorspace: %s\n",
string_for_color_space(fFormat.u.encoded_video.output.display.format));

	// allocate a buffer large enough to contain the decoded frame.
	BRect videoBounds(0.0, 0.0, width - 1, height - 1);
	fBuffer = new BBitmap(videoBounds, 0, B_YCbCr422);

	err = fBuffer->InitCheck();

	if (err < B_OK) {
		fprintf(stderr, "VideoClip::Init() - "
						"error allocating buffer: %s\n",
						strerror(err));
		delete fBuffer;
		fBuffer = NULL;
		return err;
	}

	// specifiy the decoded format. we derive this information from
	// the encoded format.
	memset(&fFormat, 0, sizeof(media_format));
	fFormat.u.raw_video.last_active = height - 1;
//	fFormat.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
//	fFormat.u.raw_video.pixel_width_aspect = 1;
//	fFormat.u.raw_video.pixel_height_aspect = 1;
	fFormat.u.raw_video.display.format = fBuffer->ColorSpace();
	fFormat.u.raw_video.display.line_width = width;
	fFormat.u.raw_video.display.line_count = height;
	fFormat.u.raw_video.display.bytes_per_row = fBuffer->BytesPerRow();

	err = fTrack->DecodedFormat(&fFormat);

	if (err < B_OK) {
		fprintf(stderr, "VideoClip::Init() - "
						"fTrack->DecodedFormat(): %s\n",
						strerror(err));
		return err;
	}

	fFrameCount = fTrack->CountFrames();

	return B_OK;
}

// GetFormat
status_t
VideoClip::GetFormat(media_format* format) const
{
	if (!fMediaFile || !fTrack)
		return B_NO_INIT;
	if (!format)
		return B_BAD_VALUE;

	*format = fFormat;

	return B_OK;
}




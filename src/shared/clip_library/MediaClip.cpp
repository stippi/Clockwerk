/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "MediaClip.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <MediaTrack.h>
#include <MediaFile.h>

#include "common.h"
#include "support.h"

#include "AudioTrackReader.h"
#include "AutoDeleter.h"
#include "CommonPropertyIDs.h"
#include "Icons.h"
#include "MediaRenderingBuffer.h"
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
MediaClip::MediaClip(const entry_ref* ref,
					 BMediaFile* file,
					 BMediaTrack* videoTrack,
					 BMediaTrack* audioTrack)
	: FileBasedClip(ref),
	  fHasVideoTrack(videoTrack != NULL),
	  fHasAudioTrack(audioTrack != NULL),
	  fBounds(0, 0, -1, -1),
	  fVideoFrameCount(0),
	  fAudioFrameCount(0),
	  fVideoFPS(0.0),
	  fAudioFPS(0.0)
{
	if (videoTrack) {
		media_format format;
		status_t ret = videoTrack->EncodedFormat(&format);
		if (ret < B_OK) {
			print_error("MediaClip::MediaClip() - "
				"videoTrack->EncodedFormat(): %s\n", strerror(ret));
			return;
		}

		// get ouput video frame size, frame count and frame rate
		fBounds = VideoBounds(format);

		fVideoFrameCount = videoTrack->CountFrames();
		fVideoFPS = format.u.encoded_video.output.field_rate
			/ format.u.encoded_video.output.interlace;
	}

	SetValue(PROPERTY_WIDTH, fBounds.IntegerWidth() + 1);
	SetValue(PROPERTY_HEIGHT, fBounds.IntegerHeight() + 1);

	if (audioTrack) {
		media_format format;
		status_t ret = audioTrack->DecodedFormat(&format);
		if (ret < B_OK) {
			fHasAudioTrack = false;
			print_error("MediaClip::MediaClip() - "
				"adioTrack->EncodedFormat(): %s\n", strerror(ret));
			return;
		}
		fAudioFrameCount = audioTrack->CountFrames();
		fAudioFPS = format.u.raw_audio.frame_rate;
	}

	_StoreArchive();
}

// constructor
MediaClip::MediaClip(const entry_ref* ref, const BMessage& archive)
	: FileBasedClip(ref),
	  fHasVideoTrack(false),
	  fHasAudioTrack(false),
	  fBounds(0, 0, -1, -1),
	  fVideoFrameCount(0),
	  fAudioFrameCount(0),
	  fVideoFPS(0.0),
	  fAudioFPS(0.0)
{
	if (archive.FindInt64("video frames", (int64*)&fVideoFrameCount) == B_OK
		&& archive.FindFloat("video fps", &fVideoFPS) == B_OK
		&& archive.FindRect("bounds", &fBounds) == B_OK)
		fHasVideoTrack = true;

	if (archive.FindInt64("audio frames", (int64*)&fAudioFrameCount) == B_OK
		&& archive.FindFloat("audio fps", &fAudioFPS) == B_OK)
		fHasAudioTrack = true;

	SetValue(PROPERTY_WIDTH, fBounds.Width() + 1);
	SetValue(PROPERTY_HEIGHT, fBounds.Height() + 1);
}

// destructor
MediaClip::~MediaClip()
{
}

// Duration
uint64
MediaClip::Duration()
{
	// TODO: this value should be passed to the function,
	// hardcoded for now
	float playlistVideoFPS = 25.0;

	if (HasVideo())
		return (uint64)(fVideoFrameCount * playlistVideoFPS / fVideoFPS);

	// convert audio rate to video rate
	if (HasAudio())
		return (uint64)(fAudioFrameCount * playlistVideoFPS / fAudioFPS);

	return 0;
}

// MaxDuration
uint64
MediaClip::MaxDuration()
{
	return Duration();
}

// HasVideo
bool
MediaClip::HasVideo()
{
	return fHasVideoTrack;
}

// HasAudio
bool
MediaClip::HasAudio()
{
	return fHasAudioTrack;
}

// CreateAudioReader
AudioReader*
MediaClip::CreateAudioReader()
{
	if (!HasAudio())
		return NULL;

	BMediaFile* mediaFile = NULL;
	BMediaTrack* audioTrack = NULL;

	status_t status = _GetMedia(Ref(), mediaFile, NULL, &audioTrack);

	ObjectDeleter<BMediaFile> fileDeleter(mediaFile);

	if (status < B_OK) {
		printf("MediaClip::_GetMedia(): %s\n", strerror(status));
		return NULL;
	}

	if (audioTrack != NULL) {
		// create new reader
		media_format format;
		format.type = B_MEDIA_RAW_AUDIO;
		format.u.raw_audio = media_raw_audio_format::wildcard;
		AudioReader* reader
			= new (nothrow) AudioTrackReader(mediaFile, audioTrack, format);
		if (reader)
			fileDeleter.Detach();
		return reader;
	}
	return NULL;
}

// Bounds
BRect
MediaClip::Bounds(BRect canvasBounds)
{
	return fBounds;
}


// GetIcon
bool
MediaClip::GetIcon(BBitmap* icon)
{
	return GetBuiltInIcon(icon, kMovieIcon);
}

// #pragma mark -

// InitCheck
status_t
MediaClip::InitCheck()
{
	if (!HasVideo() && !HasAudio())
		return B_NO_INIT;

	return B_OK;
}

// HandleReload
void
MediaClip::HandleReload()
{
	// NOTE: I think there is nothing to be done here, since
	// the Clip baseclass already increments a change counter which
	// in turn will cause the renderer to be newly instantiated
}

//	#pragma mark -

// _StoreArchive
status_t
MediaClip::_StoreArchive()
{
	BMessage archive('clip');
	status_t status = B_OK;

	if (HasVideo()) {
		if (status == B_OK)
			status = archive.AddFloat("video fps", fVideoFPS);
		if (status == B_OK)
			status = archive.AddInt64("video frames", fVideoFrameCount);
		if (status == B_OK)
			status = archive.AddRect("bounds", fBounds);
	}
	if (HasAudio()) {
		if (status == B_OK)
			status = archive.AddFloat("audio fps", fAudioFPS);
		if (status == B_OK)
			status = archive.AddInt64("audio frames", fAudioFrameCount);
	}

	if (status == B_OK)
		status = FileBasedClip::_Store("MediaClip", archive);

	return status;
}

// GetMedia
status_t
MediaClip::_GetMedia(const entry_ref* ref, BMediaFile*& mediaFile,
	BMediaTrack** _videoTrack, BMediaTrack** _audioTrack)
{
	if (_videoTrack != NULL)
		*_videoTrack = NULL;
	if (_audioTrack != NULL)
		*_audioTrack = NULL;

	mediaFile = new BMediaFile(ref);
	status_t status = mediaFile->InitCheck();
	if (status < B_OK) {
		delete mediaFile;
		mediaFile = NULL;
		return status;
	}

	// count the tracks and instantiate them, one at a time
	int32 numTracks = mediaFile->CountTracks();
	for (int32 i = 0; i < numTracks; i++) {
		BMediaTrack* track = mediaFile->TrackAt(i);
		if (track == NULL) {
			printf("cannot read media track at %ld\n", i);
			continue;
		}

		// get the encoded format
		media_format format;
		status = track->EncodedFormat(&format);
		if (status < B_OK) {
			printf("cannot understand encoded format "
				   "of track %ld: %s\n", i, strerror(status));
			continue;
		}
		bool release = true;
		switch (format.type) {
		 	case B_MEDIA_RAW_VIDEO:
		 	case B_MEDIA_ENCODED_VIDEO: {
				// video track
				if (_videoTrack != NULL && *_videoTrack == NULL) {
					*_videoTrack = track;
					release = false;
				}
				break;
		 	}
			case B_MEDIA_RAW_AUDIO:
			case B_MEDIA_ENCODED_AUDIO:
				// audio track
				if (_audioTrack != NULL && *_audioTrack == NULL) {
					*_audioTrack = track;
					release = false;
				}
				break;
			default:
				break;
		}
		if (release) {
			// didn't do anything with the track
			mediaFile->ReleaseTrack(track);
		}
	}

	return B_OK;
}

// CreateClip
Clip*
MediaClip::CreateClip(const entry_ref* ref, status_t& error)
{
	// check if this file is a MediaFile
	// instantiate a BMediaFile object, and make sure there was no error.
	BMediaFile* mediaFile;
	BMediaTrack* videoTrack;
	BMediaTrack* audioTrack;
	error = _GetMedia(ref, mediaFile, &videoTrack, &audioTrack);
	if (error < B_OK) {
		printf("MediaClip::MediaClip() - "
			   "no media file: %s\n", strerror(error));
		return NULL;
	}

	MediaClip* clip = NULL;
	if (videoTrack || audioTrack) {
		clip = new (nothrow) MediaClip(ref, mediaFile,
			videoTrack, audioTrack);
		if (clip && clip->InitCheck() < B_OK) {
			delete clip;
			clip = NULL;
		}
	}

	if (videoTrack)
		mediaFile->ReleaseTrack(videoTrack);
	if (audioTrack)
		mediaFile->ReleaseTrack(audioTrack);
	delete mediaFile;

	return clip;
}


/*static*/ BRect
MediaClip::VideoBounds(const media_format& format)
{
	const media_raw_video_format& rawformat
		= format.type == B_MEDIA_RAW_VIDEO
		? format.u.raw_video : format.u.encoded_video.output;

	int width = rawformat.display.line_width;
	int height = rawformat.display.line_count;

	// Correct for the display aspect ratio
	if (rawformat.pixel_width_aspect
			!= rawformat.pixel_height_aspect
		&& rawformat.pixel_width_aspect != 1) {
		int widthAspect = rawformat.pixel_width_aspect;
		int heightAspect = rawformat.pixel_height_aspect;
		int scaledWidth = height * widthAspect / heightAspect;
		int scaledHeight = width * heightAspect / widthAspect;
		// Use the scaling which produces an enlarged view.
		if (scaledWidth > width) {
			// Enlarge width
			width = scaledWidth;
		} else {
			// Enlarge height
			height = scaledHeight;
		}
	}

	return BRect(0, 0, width - 1, height - 1);
}


/*
 * Copyright 2000-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RenderJob.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include <Alert.h>
#include <Beep.h>
#include <Bitmap.h>
#include <Entry.h>
#include <File.h>
#include <List.h>
#include <MediaFile.h>
#include <MediaFormats.h>
#include <MediaTrack.h>
#include <NodeInfo.h>
#include <Path.h>
#include <String.h>
#include <Roster.h>

#include "support_ui.h"

#include "Playlist.h"
#include "PlaylistAudioReader.h"
#include "PlaylistRenderer.h"
#include "RenderPreset.h"
#include "RenderWindow.h"
#include "TimeCodeOverlay.h"

// debugging
#include "Debug.h"
#define ldebug debug
//# define ldebug nodebug

uint32
bytes_per_pixel(color_space format)
{
	uint32 bytes = 4;
	switch (format) {
		case B_CMAP8:
		case B_GRAY8:
			bytes = 1;
			break;
		case B_RGB15:
		case B_RGB16:
		case B_YCbCr422:
			bytes = 2;
			break;
		default:
			break;
	}
	return bytes;
}

enum {
	MSG_RENDERJOB_GO		= 'rjgo',
	MSG_WRITE_FRAME			= 'rjwf',
};

//constructor
RenderJob::RenderJob(BWindow* window, const char* docName,
		int32 startFrame, int32 endFrame, const entry_ref& ref,
		const RenderPreset* preset, float fps, Playlist* playlist)
	:
	BLooper("RenderJob", 8),
	fPlaylist(playlist),
	fRenderer(NULL),
	fAudioReader(NULL),
	fTCOverlay(NULL),
	fWidth(preset->LineWidth()),
	fHeight(preset->LineCount()),
	fFPS(fps),
	fColorSpace(preset->ColorSpace()),
	fBytesPerPixel(bytes_per_pixel(fColorSpace)),
	fMediaFile(NULL),
	fVideoTrack(NULL),
	fAudioTrack(NULL),
	fCopyRight(preset->Copyright()),

	fValidFile(false),
	fIsOpen(false),
	fHeaderCommitted(false),
	fCopyrightAdded(false),
	fRenderingAudio(false),
	fIsPaused(false),

	fRef(ref),
	fStartFrame(startFrame),
	fEndFrame(endFrame),
	fCurrentVideoFrame(fStartFrame),
	fVideoFrameForAudioPos(fStartFrame),
	fAudioPos(0),
	fStartTime(system_time()),
	fPauseStartTime(0),

	fAudioBuffer(NULL),
	fAudioFramesPerFrame(0),
	fSampleSize(0),
	fAudioChannels(preset->AudioChannelCount()),
	fAudioFrameRate(preset->AudioFrameRate())
{
	// configure file format
	media_format_family family;
	media_file_format mff;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		if (strcmp(mff.pretty_name, preset->FormatFamily().String()) == 0) {
			family = mff.family;
			break;
		}
	}
	// create the status window and configure it
	BRect frame = window->Bounds();
	frame.InsetBy(frame.Width() / 2.0 - 220.0,
				  frame.Height() /2.0 - 30.0);
	BString helper("Clockwerk - Rendering: ");
	helper << docName;
	fRenderWindow = new RenderWindow(frame, helper.String(), window, this, fWidth / fHeight);
	helper.SetTo("File: ");
	BEntry entry(&fRef);
	BPath path;
	if (entry.GetPath(&path) == B_OK)
		helper << path.Path();
	else
		helper << fRef.name;
	if (fRenderWindow->Lock()) {
		fRenderWindow->SetCurrentFileInfo(helper.String());
		frame = fRenderWindow->Frame();
		center_frame_on_screen(frame, fRenderWindow);
		fRenderWindow->MoveTo(frame.LeftTop());
		fRenderWindow->ResizeTo(frame.Width(), frame.Height());
		fRenderWindow->Show();
		fRenderWindow->Unlock();
	}

	// Create the file, using the given media file format,
	status_t err = _CreateFile(fRef, mff);
	if (err != B_OK) {
		fprintf(stderr, "ERROR:  _CreateFile() returned 0x%lx (%s)\n", err,
			strerror(err));
		fValidFile = false;
	} else {
		// add copyright before adding any tracks
		if ((err = fMediaFile->AddCopyright(fCopyRight.String())) < B_OK)
			printf("ERROR: failed to add copyright \"%s\": %s\n",
				fCopyRight.String(), strerror(err));
		// if an error occured, ignore it
		err = B_OK;
		media_format format, outFormat;
		memset(&format, 0, sizeof(media_format));
		status_t videoError = B_ERROR;
		status_t audioError = B_ERROR;
		if (preset->IsVideo()) {
			uint32 flags = 0;

			fRenderer = new PlaylistRenderer(fPlaylist,
				(uint32)fWidth, (uint32)fHeight, flags, fColorSpace);
			const BBitmap* bitmap = fRenderer->Bitmap();
			// configure video format
			format.type = B_MEDIA_RAW_VIDEO;
			format.u.raw_video.display.line_width = (uint32)fWidth;
			format.u.raw_video.display.line_count = (uint32)fHeight;
			format.u.raw_video.last_active
				= format.u.raw_video.display.line_count - 1;
			format.u.raw_video.orientation = B_VIDEO_TOP_LEFT_RIGHT;
			format.u.raw_video.display.format = bitmap->ColorSpace();
			format.u.raw_video.display.bytes_per_row
				= bitmap ? bitmap->BytesPerRow() : 0;
			format.u.raw_video.interlace = 1;
			format.u.raw_video.field_rate = fps;
			format.u.raw_video.pixel_width_aspect = 1;
			format.u.raw_video.pixel_height_aspect = 1;

			if (fRenderer->IsValid()) {
				// create video track if all went well
				cookie = 0;
				bool foundCodec = false;
				while (get_next_encoder(&cookie, &mff, &format, &outFormat,
						&fVideoCodecInfo) == B_OK) {
					if (strcmp(fVideoCodecInfo.pretty_name,
							preset->VideoCodec().String()) == 0) {
						foundCodec = true;
						break;
					}
				}
				if (foundCodec) {
					videoError = _CreateVideoTrack(format, fVideoCodecInfo,
						preset->VideoQuality());
				} else {
					fprintf(stderr, "video codec not found! (%s)\n",
						preset->VideoCodec().String());
				}
				if (videoError != B_OK) {
					BString errorMessage("Error creating video track of media "
						"file:\n\n");
					errorMessage << strerror(videoError);
					(new BAlert("show stopper", errorMessage.String(), "Darn",
						NULL, NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING,
						B_STOP_ALERT))->Go();
					fprintf(stderr, "error creating video track! (%s)\n",
						strerror(videoError));
				}
			} else {
				fprintf(stderr, "error creating xsheet video renderer!\n");
				delete fRenderer;
				fRenderer = NULL;
			}
		}
		memset(&format, 0, sizeof(media_format));
		if (preset->IsAudio()) {
			// "calculate" buffer size
			fAudioFramesPerFrame = 4096;

			format.type = B_MEDIA_RAW_AUDIO;
			format.u.raw_audio.channel_count = fAudioChannels;
			format.u.raw_audio.frame_rate = fAudioFrameRate;
			format.u.raw_audio.byte_order = B_MEDIA_HOST_ENDIAN;
			format.u.raw_audio.format = media_raw_audio_format::B_AUDIO_SHORT;

			fSampleSize = format.u.raw_audio.format
				& media_raw_audio_format::B_AUDIO_SIZE_MASK;
			format.u.raw_audio.buffer_size = fAudioFramesPerFrame
				* fAudioChannels * fSampleSize;
			// create audio reader
			fAudioReader = new PlaylistAudioReader(fPlaylist, NULL, format,
				fps);
			if (fAudioReader->InitCheck() == B_OK) {
				// create track if all went well
				media_codec_info audio_codec;
				cookie = 0;
				bool foundCodec = false;
				while (get_next_encoder(&cookie, &mff, &format, &outFormat,
						&audio_codec) == B_OK) {
					if (strcmp(audio_codec.pretty_name,
							preset->AudioCodec().String()) == 0) {
						foundCodec = true;
						break;
					}
				}
				if (foundCodec)
					audioError = _CreateAudioTrack(format, audio_codec,
						preset->AudioQuality());
				else {
					fprintf(stderr, "audio codec not found! (%s)\n",
						preset->AudioCodec().String());
				}
				if (audioError == B_OK) {
					// create the audio buffer
					fAudioBuffer = new uint8[fAudioFramesPerFrame
						* fAudioChannels * fSampleSize];
					fAudioPos = fAudioReader->AudioFrameForVideoFrame(
						fCurrentVideoFrame);
					fAudioEndPos = fAudioReader->AudioFrameForVideoFrame(
						fEndFrame + 1) - 1;
					if (fAudioEndPos < fAudioPos + fAudioFramesPerFrame)
						fAudioFramesPerFrame = fAudioEndPos - fAudioPos + 1;
				} else {
					BString errorMessage("Error creating audio track of media "
						"file:\n\n");
					errorMessage << strerror(audioError);
					(new BAlert("show stopper", errorMessage.String(), "Darn",
						NULL, NULL, B_WIDTH_AS_USUAL, B_EVEN_SPACING,
						B_STOP_ALERT))->Go();
					fprintf(stderr, "error creating audio track! (%s)\n",
						strerror(audioError));
				}
			} else {
				fprintf(stderr, "error creating xsheet audio reader!\n");
				delete fAudioReader;
				fAudioReader = NULL;
			}
		}
		if (videoError >= B_OK || audioError >= B_OK)
			fValidFile = true;
		else
			fprintf(stderr, "ERROR:  The creation of both tracks failed!\n");
	}
	if (!fVideoTrack)
		fRenderingAudio = true;

	// time code support
	if (preset->TimeCodeOverlay()) {
		float overlaySize = (fWidth * preset->TimeCodeScale()) / 50.0;
		fTCOverlay = new TimeCodeOverlay(overlaySize);
		fTCOverlay->SetTransparency(preset->TimeCodeTransparency());
	}
}

// destructor
RenderJob::~RenderJob()
{
	// If the movie is still opened, close it; this also flushes all tracks
	if (fMediaFile && fIsOpen)
		_CloseFile();
	delete fMediaFile;		// deletes the track, too
	if (fRenderWindow && fRenderWindow->Lock()) {
		if (fRenderWindow->BeepWhenDone())
			beep();
		if (fRenderWindow->OpenMovieWhenDone())
			be_roster->Launch(&fRef);
		fRenderWindow->Quit();
	}
	delete[] fAudioBuffer;
	delete fRenderer;
	delete fAudioReader;
	delete fPlaylist;
}

// MessageReceived
void RenderJob::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		case MSG_RENDERJOB_GO:
			fStartTime = system_time();
			PostMessage(MSG_WRITE_FRAME, this);
			break;

		case MSG_RENDER_WINDOW_PAUSE:
			fIsPaused = !fIsPaused;
			if (!fIsPaused) {
				PostMessage(MSG_WRITE_FRAME);
				// don't take paused time into estimated finish consideration
				bigtime_t timePaused = system_time() - fPauseStartTime;
				fStartTime -= timePaused;
			} else
				// remember time we go into paused mode (see above)
				fPauseStartTime = system_time();
			if (fRenderWindow->Lock()) {
				fRenderWindow->SetPaused(fIsPaused);
				fRenderWindow->Unlock();
			}
			break;

		case MSG_WRITE_FRAME:
		{
			if (fIsPaused)
				break;
			if (!fValidFile) {
				PostMessage(B_QUIT_REQUESTED, this);
				break;
			}

			status_t err = _WriteAudio();
			if (err == B_OK)
				err = _WriteVideo();
			// update status window
			_UpdateRenderWindow();
			// generate next frame or quit if done/error
			if (err != B_OK || fCurrentVideoFrame >= fEndFrame)
				PostMessage(B_QUIT_REQUESTED, this);
			else
				PostMessage(MSG_WRITE_FRAME, this);
			break;
		}
		case MSG_RENDER_WINDOW_CANCEL:
			PostMessage(B_QUIT_REQUESTED, this);
			fCurrentVideoFrame = fEndFrame + 1L;
			break;
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}

// QuitRequested
bool
RenderJob::QuitRequested()
{
	// close the file
	status_t closeErr = _CloseFile();
	if (closeErr != B_OK)
		fprintf(stderr, "ERROR:  _CloseFile() returned 0x%lx (%s)\n",
				closeErr, strerror(closeErr));
	return true;
}

// Go
void
RenderJob::Go()
{
	Run();
	PostMessage(MSG_RENDERJOB_GO);
}

// #pragma mark -

// _CreateFile
status_t
RenderJob::_CreateFile(const entry_ref &fRef, const media_file_format &mff)
{
	BMediaFile* file = new BMediaFile(&fRef, &mff);
	status_t err = file->InitCheck();
	if (err == B_OK) {
		fIsOpen = true;
		fMediaFile = file;
	} else {
		// clean up if we incurred an error
		fIsOpen = false;
		if (fMediaFile) {
			delete fMediaFile;
			fMediaFile = NULL;
		}
	}
	BNode node(&fRef);
	if (node.InitCheck() == B_OK) {
		BNodeInfo node_info(&node);
		if (node_info.InitCheck() == B_OK)
			node_info.SetType(mff.mime_type);
	}
	return err;
}

// _CommitHeader
status_t
RenderJob::_CommitHeader()
{
	if (fHeaderCommitted)
		return B_OK;

	status_t ret = fMediaFile->CommitHeader();
	if (ret == B_OK)
		fHeaderCommitted = true;
	else {
		fprintf(stderr, "RenderJob::_CommitHeader(): Could not commit "
			"header to media file: %s\n", strerror(ret));
	}

	return ret;
}

// _CreateVideoTrack
status_t
RenderJob::_CreateVideoTrack(const media_format& videoInputFormat,
	const media_codec_info& videoCodec, float quality)
{
	status_t err = B_OK;
	if (fMediaFile) {
		// This next line casts away const to avoid a warning.
		// MediaFile::CreateTrack() *should* have the input format argument
		// declared const, but it doesn't, and it can't be changed because
		// it would break binary compatibility. Oh, well.
		fVideoTrack = fMediaFile->CreateTrack(
			(media_format*)&videoInputFormat, &videoCodec);
		if (!fVideoTrack)
			err = B_ERROR;
		else {
			if (quality >= 0.0f)
				fVideoTrack->SetQuality(quality);
		}
	}
	return err;
}

// CreatAudioTrack
status_t
RenderJob::_CreateAudioTrack(const media_format& audioInputFormat,
	const media_codec_info& audioCodec, float quality)
{
	status_t err = B_OK;
	if (fMediaFile) {
		// This next line casts away const to avoid a warning.
		// MediaFile::CreateTrack() *should* have the input format argument
		// declared const, but it doesn't, and it can't be changed because
		// it would break binary compatibility. Oh, well.
		fAudioTrack = fMediaFile->CreateTrack(
			(media_format*)&audioInputFormat, &audioCodec);
		if (!fAudioTrack)
			err = B_ERROR;
		else {
			if (quality >= 0.0f)
				fAudioTrack->SetQuality(quality);
		}
	}
	return err;
}

// _WriteVideo
status_t
RenderJob::_WriteVideo()
{
	if (fVideoTrack == NULL) {
		fCurrentVideoFrame = fVideoFrameForAudioPos;
		return B_OK;
	}

	if (fCurrentVideoFrame > fEndFrame)
		return B_OK;

	status_t err;
	if (fTCOverlay) {
		bool wasCached;
		err = fRenderer->RenderFrame(fCurrentVideoFrame, wasCached);
		if (err == B_OK) {
			BBitmap temp(fRenderer->Bitmap(), B_BITMAP_NO_SERVER_LINK);
			fTCOverlay->DrawTimeCode(&temp, fCurrentVideoFrame, fFPS);
			err = _WriteVideo(&temp, false);
			_UpdateRenderWindow(&temp);
		}
	} else {
		const void* chunkBuffer;
		size_t chunkSize;
		bool chunksComplete;
		int32 chunksWritten = 0;
		while ((err = fRenderer->GetNextVideoChunk(fCurrentVideoFrame,
				chunkBuffer, chunkSize, chunksComplete, fVideoCodecInfo))
				== B_OK) {
			if (chunksComplete) {
printf("frame %ld - chunks written: %ld\n", fCurrentVideoFrame, chunksWritten);
				break;
			}
			chunksWritten++;

			err = _WriteVideoChunk(chunkBuffer, chunkSize,
				(fRenderer->ChunkHeader().u.encoded_video
					.field_flags & B_MEDIA_KEY_FRAME) != 0);
			if (err != B_OK)
				break;
			// TODO: Find a way to update the render window when not
			// reencoding.
		}
		if (chunksWritten == 0) {
printf("frame %ld - re-encoded\n", fCurrentVideoFrame);
			// No chunks could be written non-destructively,
			// we need to re-encode the video.
			bool wasCached;
			err = fRenderer->RenderFrame(fCurrentVideoFrame,
				wasCached);
			if (err == B_OK) {
				err = _WriteVideo(fRenderer->Bitmap(),
					!wasCached);
			}
			_UpdateRenderWindow(fRenderer->Bitmap());
		}
	}

	if (err != B_OK) {
		BString errorMessage("Error writing video to media "
			"file:\n\n");
		errorMessage << strerror(err);
		int32 ret = (new BAlert("show stopper",
			errorMessage.String(), "Retry", "Give up", NULL,
			B_WIDTH_AS_USUAL, B_EVEN_SPACING,
			B_STOP_ALERT))->Go();
		if (ret == 0) {
			// Try again next time
			fCurrentVideoFrame--;
			err = B_OK;
		}
	}

	fCurrentVideoFrame++;

	return err;
}

// _WriteVideo
status_t
RenderJob::_WriteVideo(const BBitmap* bitmap, bool isKeyFrame)
{
	// NULL is not a valid bitmap pointer
	if (!bitmap)
		return B_BAD_VALUE;

	// if there's no track, this won't work
	if (!fVideoTrack)
		return B_NO_INIT;

	// Verify that the bitmap is the right dimensions and that it has the
	// right color space.
	BRect r = bitmap->Bounds();
	if ((r.Width() != fWidth - 1)
		|| (r.Height() != fHeight - 1)
		|| (bitmap->ColorSpace() != fColorSpace)) {
		return B_MISMATCHED_VALUES;
	}

	// Okay, it's the right kind of bitmap -- commit the header if necessary,
	// and write it as one video frame.  We defer committing the header until
	// the first frame is written in order to allow the client to adjust the
	// image quality at any time up to actually writing video data.
	status_t err = _CommitHeader();
	if (err != B_OK)
		return err;

	err = fVideoTrack->WriteFrames(bitmap->Bits(), 1, isKeyFrame ?
		B_MEDIA_KEY_FRAME : 0);

//	uint32 length = (uint32)fWidth * fBytesPerPixel;
//	uint8* buffer = new uint8[length * (uint32)fHeight];
//	uint8* dest = buffer;
//	uint8* src = (uint8*)bitmap->Bits();
//	uint32 bpr = bitmap->BytesPerRow();
//	for (uint32 i = 0; i < (uint32)fHeight; i++) {
//		memcpy(dest, src, length);
//		dest += length;
//		src += bpr;
//	}
//	err = fVideoTrack->WriteFrames(buffer, 1, (isKeyFrame) ?
//		B_MEDIA_KEY_FRAME : 0);
//	delete[] buffer;

	return err;
}

// _WriteVideoChunk
status_t
RenderJob::_WriteVideoChunk(const void* buffer, size_t size, bool isKeyFrame)
{
	if (buffer == NULL || size == 0)
		return B_BAD_VALUE;

	if (fVideoTrack == NULL)
		return B_NO_INIT;

	// Commit the header if necessary. We defer committing the header until
	// the first frame is written in order to allow the client to adjust the
	// image quality at any time up to actually writing video data.
	status_t err = _CommitHeader();
	if (err != B_OK)
		return err;

	err = fVideoTrack->WriteChunk(buffer, size, isKeyFrame ?
		B_MEDIA_KEY_FRAME : 0);

	return err;
}

// _WriteAudio
status_t
RenderJob::_WriteAudio()
{
	if (fAudioTrack == NULL || fAudioReader == NULL)
		return B_OK;

	if (fAudioPos > fAudioEndPos)
		return B_OK;

	// Write as many audio buffers as needed to catch up
	// with the last written video frame.
	while (fVideoFrameForAudioPos <= fCurrentVideoFrame) {
		int64 framesToWrite = fAudioEndPos - fAudioPos + 1;
		if (framesToWrite > fAudioFramesPerFrame)
			framesToWrite = fAudioFramesPerFrame;

		if (fAudioReader->Read(fAudioBuffer, fAudioPos,
				framesToWrite) != B_OK) {
			fprintf(stderr, "ERROR:  RenderAudio failed - "
				"Rendering silence!!!\n");
			// make audio buffer empty
			memset(fAudioBuffer, 0, framesToWrite
				* fAudioChannels * fSampleSize);
		}
		status_t err = _WriteAudio(fAudioBuffer, framesToWrite);
		if (err != B_OK) {
			BString errorMessage("Error writing audio to media file:\n\n");
			errorMessage << strerror(err);
			int32 ret = (new BAlert("show stopper",
				errorMessage.String(), "Retry", "Give up", NULL,
				B_WIDTH_AS_USUAL, B_EVEN_SPACING,
				B_STOP_ALERT))->Go();
			if (ret != 0)
				return ret;
		}
		fAudioPos += framesToWrite;
		fVideoFrameForAudioPos = fAudioReader->VideoFrameForAudioFrame(
			fAudioPos);
	}
	return B_OK;
}

// _WriteAudio
status_t
RenderJob::_WriteAudio(void* data, int32 frames)
{
	// NULL is not a valid buffer pointer
	if (!data)
		return B_BAD_VALUE;

	// if there's no track, this won't work
	if (!fAudioTrack)
		return B_NO_INIT;

	status_t err = _CommitHeader();
	if (err != B_OK)
		return err;

	err = fAudioTrack->WriteFrames(data, frames, B_MEDIA_KEY_FRAME);
	return err;
}

// _CloseFile
status_t
RenderJob::_CloseFile()
{
	if (!fMediaFile)
		return B_OK;

	status_t err = B_OK;
	if (fIsOpen) {
		if (fVideoTrack) {
			if ((err = fVideoTrack->Flush()) < B_OK) {
				fprintf(stderr, "RenderJob::_CloseFile() - Could not "
					"flush video track: %s\n", strerror(err));
			}
			fMediaFile->ReleaseTrack(fVideoTrack);
		}
		if (fAudioTrack) {
			if ((err = fAudioTrack->Flush()) < B_OK) {
				fprintf(stderr, "RenderJob::_CloseFile() - Could not "
					"flush audio track: %s\n", strerror(err));
			}
			fMediaFile->ReleaseTrack(fAudioTrack);
		}
		err = fMediaFile->CloseFile();
		// keep track of whether the file is open so that we don't call
		// _CloseFile() twice; this avoids a bug in some current (R4.5.1)
		// file writers.
		fIsOpen = false;
	}
	delete fMediaFile;
	fMediaFile = NULL;
	fVideoTrack = NULL;
	fAudioTrack = NULL;

	return err;
}

// _UpdateRenderWindow
void RenderJob::_UpdateRenderWindow(const BBitmap* bitmap)
{
	if (!fRenderWindow->Lock())
		return;

	if (bitmap) {
		// just draw the bitmap and be done with it
		if (!fRenderingAudio && fRenderer)
			fRenderWindow->DrawCurrentFrame(bitmap);
	} else {
		BString leftLabel("Rendering ");
		if (!fRenderingAudio)
			leftLabel << "video track";
		else
			leftLabel << "audio track";
		BString rightLabel("Frame: ");
		rightLabel << fCurrentVideoFrame + 1L << "  (" << fEndFrame
			- fCurrentVideoFrame << " to go)";
		fRenderWindow->SetStatusBarInfo(leftLabel.String(),
			rightLabel.String(), fEndFrame - fStartFrame,
			fCurrentVideoFrame - fStartFrame);
		// guestimate finish
		if (fCurrentVideoFrame > fStartFrame) {
			bigtime_t elapsed = system_time() - fStartTime;
			bigtime_t timePerFrame = elapsed / (fCurrentVideoFrame
				- fStartFrame);
			time_t finish = (time_t)real_time_clock()
				+ (time_t)((timePerFrame * (fEndFrame - fCurrentVideoFrame - 1))
				/ 1000000);

			tm* time = localtime(&finish);
			int32 year = time->tm_year;
			if (year >= 100)
				year = 2000 + year - 100;
			else
				year += 1900;
			char timeText[20];
			if ((time_t)real_time_clock() < finish - 24 * 60 * 60) {
				// rendering is going to take more than a day!
				sprintf(timeText, "%0*d:%0*d:%0*d %0*d/%0*d/%ld",
					2, time->tm_hour, 2, time->tm_min, 2, time->tm_sec,
					2, time->tm_mon + 1, 2, time->tm_mday, year);
			} else {
				sprintf(timeText, "%0*d:%0*d:%0*d",
					2, time->tm_hour, 2, time->tm_min, 2, time->tm_sec);
			}

			BString helper("Guestimated finish: ");
			helper << timeText << "    (";
			finish -= (time_t)real_time_clock();
			time = gmtime(&finish);

			sprintf(timeText, "%0*d:%0*d:%0*d", 2, time->tm_hour, 2,
				time->tm_min, 2, time->tm_sec);
			helper << timeText << " to go)";
			fRenderWindow->SetTimeText(helper.String());
		}
	}
	fRenderWindow->Unlock();
}

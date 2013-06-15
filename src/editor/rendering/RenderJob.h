/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef RENDERJOB_H
#define RENDERJOB_H

#include <Looper.h>

#include <Entry.h>
#include <GraphicsDefs.h>
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <String.h>

class BBitmap;
class BMediaFile;
class BMediaTrack;
class BWindow;
class Playlist;
class PlaylistAudioReader;
class PlaylistRenderer;
class RenderWindow;
class RenderPreset;
class TimeCodeOverlay;

class RenderJob : public BLooper {
public:
								RenderJob(BWindow* window, const char* docName,
									int32 startFrame, int32 endFrame,
									const entry_ref& ref,
									const RenderPreset* preset, float fps,
									Playlist* playlist);
	virtual						~RenderJob();

	// BLooper interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	// RenderJob
			void				Go();

private:
			void				_UpdateRenderWindow(
									const BBitmap* bitmap = NULL);

			status_t			_CreateFile(const entry_ref& file,
									const media_file_format& fileFormat);

			status_t			_CommitHeader();

			status_t			_CreateVideoTrack(
									const media_format& videoInputFormat,
									const media_codec_info& videoCodec,
									float quality = -1.0);
										// [0,1]; negative number means use
										// default quality

			status_t			_CreateAudioTrack(
									const media_format& audioInputFormat,
									const media_codec_info& audioCodec,
									float quality = -1.0);
										// [0,1]; negative number means use
										// default quality

								// Write a BBitmap as one frame of the file
			status_t			_WriteVideo();
			status_t			_WriteVideo(const BBitmap* bitmap,
									bool isKeyFrame = false);
			status_t			_WriteVideoChunk(const void* buffer,
									size_t size, bool isKeyFrame = false);
								// Write audio data
			status_t			_WriteAudio();
			status_t			_WriteAudio(void* buffer, int32 samples);

			status_t			_CloseFile();

private:
			Playlist*			fPlaylist;
			PlaylistRenderer*	fRenderer;
			PlaylistAudioReader* fAudioReader;
			TimeCodeOverlay*	fTCOverlay;
			float				fWidth;
			float				fHeight;
			float				fFPS;
			color_space			fColorSpace;
			uint32				fBytesPerPixel;
			BMediaFile*			fMediaFile;
			BMediaTrack*		fVideoTrack;
			BMediaTrack*		fAudioTrack;
			BString				fCopyRight;
			media_codec_info	fVideoCodecInfo;

			bool				fValidFile;
			bool				fIsOpen;
			bool				fHeaderCommitted;
			bool				fVideoFlushed;
			bool				fAudioFlushed;
			bool				fCopyrightAdded;
			bool				fRenderingAudio;
			bool				fIsPaused;

			RenderWindow*		fRenderWindow;

			entry_ref			fRef;
			int32				fStartFrame;
			int32				fEndFrame;
			int32				fCurrentVideoFrame;
			int32				fVideoFrameForAudioPos;
			int64				fAudioPos;
			int64				fAudioEndPos;
			bigtime_t			fStartTime;
			bigtime_t			fPauseStartTime;

			uint8*				fAudioBuffer;
			int32				fAudioFramesPerFrame;
			int32				fSampleSize;
			uint32				fAudioChannels;
			float				fAudioFrameRate;
};

#endif // RENDERJOB_H

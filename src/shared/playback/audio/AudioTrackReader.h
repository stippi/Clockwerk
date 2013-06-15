/*
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef AUDIO_TRACK_READER_H
#define AUDIO_TRACK_READER_H

#include <List.h>

#include "AudioReader.h"

class BMediaFile;
class BMediaTrack;

class AudioTrackReader : public AudioReader {
 public:
								AudioTrackReader(BMediaTrack* mediaTrack,
									const media_format& format);
								AudioTrackReader(BMediaFile* mediaFile,
									BMediaTrack* mediaTrack,
									const media_format& format);
	virtual						~AudioTrackReader();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

			BMediaTrack*		MediaTrack() const;

 private:
			struct Buffer;
			void				_InitFromTrack();

			int64				_FramesPerBuffer() const;

			void				_CopyFrames(void* source, int64 sourceOffset,
									void* target, int64 targetOffset,
									int64 position, int64 frames) const;
			void				_CopyFrames(Buffer* buffer, void* target,
									int64 targetOffset, int64 position,
									int64 frames) const;

			void				_AllocateBuffers();
			void				_FreeBuffers();
			Buffer*				_BufferAt(int32 index) const;
			Buffer*				_FindBufferAtFrame(int64 frame) const;
			Buffer*				_FindUnusedBuffer() const;
			Buffer*				_FindUsableBuffer() const;
			Buffer*				_FindUsableBufferFor(int64 position) const;
			void				_GetBuffersFor(BList& buffers, int64 position,
									int64 frames) const;
			void				_TouchBuffer(Buffer* buffer);

			status_t			_ReadBuffer(Buffer* buffer, int64 position);
			status_t			_ReadBuffer(Buffer* buffer, int64 position,
									bigtime_t time);

			void				_ReadCachedFrames(void*& buffer,
									int64& position, int64& frames);
			void				_ReadCachedFrames(void*& buffer,
									int64& position, int64& frames,
									bigtime_t time);

			status_t			_ReadUncachedFrames(void* buffer,
									int64 position, int64 frames);
			status_t			_ReadUncachedFrames(void* buffer,
									int64 position, int64 frames,
									bigtime_t time);

			status_t			_FindKeyFrameForward(int64& position);
			status_t			_FindKeyFrameBackward(int64& position);
			status_t			_SeekToKeyFrameForward(int64& position);
			status_t			_SeekToKeyFrameBackward(int64& position);

 private:
			BMediaFile*			fMediaFile;
			BMediaTrack*		fMediaTrack;
			char*				fBuffer;
			int64				fBufferOffset;
			int64				fBufferSize;
			BList				fBuffers;
			bool				fHasKeyFrames;
			int64				fCountFrames;
			bool				fReportSeekError;
};

#endif	// AUDIO_TRACK_READER_H

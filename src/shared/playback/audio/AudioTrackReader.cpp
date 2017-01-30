/*
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <algorithm>
#include <string.h>

#include <MediaFile.h>
#include <MediaTrack.h>

#include "AudioTrackReader.h"

// debugging
#include "Debug.h"
//#define ldebug debug
#define ldebug nodebug


// Buffer

struct AudioTrackReader::Buffer {
			void*				data;
			int64				offset;
			int64				size;
			bigtime_t			time_stamp;

	static	int					CompareOffset(const void* a, const void* b);
};

// CompareOffset
int
AudioTrackReader::Buffer::CompareOffset(const void* a, const void* b)
{
	const Buffer* buffer1 = *(const Buffer**)a;
	const Buffer* buffer2 = *(const Buffer**)b;
	int result = 0;
	if (buffer1->offset < buffer2->offset)
		result = -1;
	else if (buffer1->offset > buffer2->offset)
		result = 1;
	return result;
}


// AudioTrackReader

// constructor
AudioTrackReader::AudioTrackReader(BMediaTrack* mediaTrack,
		const media_format& format)
	: AudioReader(format),
	  fMediaFile(NULL),
	  fMediaTrack(mediaTrack),
	  fBuffer(NULL),
	  fBufferOffset(0),
	  fBufferSize(0),
	  fBuffers(10),
	  fHasKeyFrames(false),
	  fCountFrames(0),
	  fReportSeekError(true)
{
	_InitFromTrack();
}

// constructor
AudioTrackReader::AudioTrackReader(BMediaFile* mediaFile,
		BMediaTrack* mediaTrack, const media_format& format)
	: AudioReader(format),
	  fMediaFile(mediaFile),
	  fMediaTrack(mediaTrack),
	  fBuffer(NULL),
	  fBufferOffset(0),
	  fBufferSize(0),
	  fBuffers(10),
	  fHasKeyFrames(false),
	  fCountFrames(0),
	  fReportSeekError(true)
{
	_InitFromTrack();
}

// destructor
AudioTrackReader::~AudioTrackReader()
{
	_FreeBuffers();
	delete[] fBuffer;

	if (fMediaFile) {
		// the track and file belong to us
		if (fMediaTrack)
			fMediaFile->ReleaseTrack(fMediaTrack);
		delete fMediaFile;
	}
}

// Read
status_t
AudioTrackReader::Read(void* buffer, int64 pos, int64 frames)
{
ldebug("AudioTrackReader::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);
ldebug("  this: %p, fOutOffset: %Ld\n", this, fOutOffset);
	status_t error = InitCheck();
	if (error != B_OK)
{
ldebug("AudioTrackReader::Read() done\n");
		return error;
}
	// convert pos according to our offset
	pos += fOutOffset;
	// Fill the frames after the end of the track with silence.
	if (pos + frames > fCountFrames) {
		int64 size = std::max((int64)0, fCountFrames - pos);
		ReadSilence(SkipFrames(buffer, size), frames - size);
		frames = size;
	}
ldebug("  after eliminating the frames after the track end: %p, %Ld, %Ld\n",
buffer, pos, frames);
	// read the cached frames
	bigtime_t time = system_time();
	if (frames > 0)
		_ReadCachedFrames(buffer, pos, frames, time);
ldebug("  after reading from cache: %p, %Ld, %Ld\n", buffer, pos, frames);
	// read the remaining (uncached) frames
	if (frames > 0)
		_ReadUncachedFrames(buffer, pos, frames, time);
ldebug("AudioTrackReader::Read() done\n");
	return B_OK;

/*
	int64 sampleSize = fFormat.u.raw_audio.format 
					   & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	int64 framesPerBuffer = fFormat.u.raw_audio.buffer_size / frameSize;
	// If our buffer contains either a part of the beginning or of the end,
	// we copy it to the supplied buffer and alter pos/frames.
	if (fBufferSize > 0) {
		if (fBufferOffset <= pos && fBufferOffset + fBufferSize > pos) {
			int64 size = min(frames, fBufferOffset + fBufferSize - pos);
//			memcpy(buffer,
//				   fBuffer + (pos - fBufferOffset) * frameSize,
//				   size * frameSize);
			_CopyFrames(fBuffer, fBufferOffset, buffer, pos, pos, size);
			pos += size;
			frames -= size;
			buffer = _SkipFrames(buffer, size);
		} else if (fBufferOffset < pos + frames
				   && fBufferOffset + fBufferSize >= pos + frames) {
			int64 size = min(frames, pos + frames - fBufferOffset);
//			memcpy((char*)buffer + (frames - size) * frameSize,
//				   fBuffer + (pos + frames - size - fBufferOffset) * frameSize,
//				   size * frameSize);
			_CopyFrames(fBuffer, fBufferOffset, buffer, pos,
						pos + frames - size, size);
			frames -= size;
		}
	}
	// read the remaining frames from the media track
	// seek to the position
	int64 currentPos = pos;
	if (frames > 0) {
		error = fMediaTrack->SeekToFrame(&currentPos,
										 B_MEDIA_SEEK_CLOSEST_BACKWARD);
ldebug("  seeked to position: %Ld\n", currentPos);
	}
	while (error == B_OK && frames > 0) {
		if (frames < framesPerBuffer || pos != currentPos) {
			int64 readFrames = 0;
			error = fMediaTrack->ReadFrames(fBuffer, &readFrames);
ldebug("  BMediaTrack::ReadFrames(%p): read %Ld frames\n", fBuffer, readFrames);
			if (error == B_OK) {
				if (currentPos + readFrames > pos) {
					int64 size = min(frames, readFrames - pos + currentPos);
					memcpy(buffer, fBuffer, size * frameSize);
					buffer = (char*)buffer + size * frameSize;
					frames -= size;
				} else if (readFrames == 0)
					break;
				fBufferOffset = currentPos;
				fBufferSize = readFrames;
				currentPos += readFrames;
			}
		} else {
			int64 readFrames = 0;
			error = fMediaTrack->ReadFrames(buffer, &readFrames);
ldebug("  BMediaTrack::ReadFrames(%p): read %Ld frames\n", buffer, readFrames);
			if (error == B_OK) {
				if (readFrames > 0) {
					buffer = _SkipFrames(buffer, readFrames);
					frames -= readFrames;
				} else
					break;
				currentPos += readFrames;
			}
		}
	}
	// fill up the rest with 0
	if (frames > 0)
		memset(buffer, 0, frames * frameSize);
ldebug("AudioTrackReader::Read() done\n");
	return B_OK;
*/
}

// InitCheck
status_t
AudioTrackReader::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && (!fMediaTrack || !fBuffer))
		error = B_NO_INIT;
	return error;
}

// MediaTrack
BMediaTrack*
AudioTrackReader::MediaTrack() const
{
	return fMediaTrack;
}

// #pragma mark -

// _InitFromTrack
void
AudioTrackReader::_InitFromTrack()
{
	if (fMediaTrack && fMediaTrack->DecodedFormat(&fFormat) == B_OK
		&& fFormat.type == B_MEDIA_RAW_AUDIO) {
char formatString[256];
string_for_format(fFormat, formatString, 256);
ldebug("AudioTrackReader: format is: %s\n", formatString);
		fBuffer = new char[fFormat.u.raw_audio.buffer_size];
		_AllocateBuffers();
		// Find out, if the track has key frames: as a heuristic we
		// check, if the first and the second frame have the same backward
		// key frame.
		// Note: It shouldn't harm that much, if we're wrong and the
		// track has key frame although we found out that it has not.
		int64 keyFrame0 = 0;
		int64 keyFrame1 = 1;
		fMediaTrack->FindKeyFrameForFrame(&keyFrame0,
										  B_MEDIA_SEEK_CLOSEST_BACKWARD);
		fMediaTrack->FindKeyFrameForFrame(&keyFrame1,
										  B_MEDIA_SEEK_CLOSEST_BACKWARD);
		fHasKeyFrames = (keyFrame0 == keyFrame1);
//fHasKeyFrames = false;
		// get the length of the track
		fCountFrames = fMediaTrack->CountFrames();
	} else
		fMediaTrack = NULL;
}

// _FramesPerBuffer
int64
AudioTrackReader::_FramesPerBuffer() const
{
	int64 sampleSize = fFormat.u.raw_audio.format 
					   & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	return fFormat.u.raw_audio.buffer_size / frameSize;
}

// _CopyFrames
//
// Given two buffers starting at different frame offsets, this function
// copies /frames/ frames at position /position/ from the source to the
// target buffer.
// Note that no range checking is done.
void
AudioTrackReader::_CopyFrames(void* source, int64 sourceOffset,
							  void* target, int64 targetOffset,
							  int64 position, int64 frames) const
{
	int64 sampleSize = fFormat.u.raw_audio.format 
					   & media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	source = (char*)source + frameSize * (position - sourceOffset);
	target = (char*)target + frameSize * (position - targetOffset);
	memcpy(target, source, frames * frameSize);
}

// _CopyFrames
//
// Given two buffers starting at different frame offsets, this function
// copies /frames/ frames at position /position/ from the source to the
// target buffer. This version expects a cache buffer as source.
// Note that no range checking is done.
void
AudioTrackReader::_CopyFrames(Buffer* buffer,
							  void* target, int64 targetOffset,
							  int64 position, int64 frames) const
{
	_CopyFrames(buffer->data, buffer->offset, target, targetOffset, position,
				frames);
}

// _AllocateBuffers
//
// Allocates a set of buffers.
void
AudioTrackReader::_AllocateBuffers()
{
	int32 count = 10;
	_FreeBuffers();
	int32 bufferSize = fFormat.u.raw_audio.buffer_size;
	char* data = new char[bufferSize * count];
	for (; count > 0; count--) {
		Buffer* buffer = new Buffer;
		buffer->data = data;
		data += bufferSize;
		buffer->offset = 0;
		buffer->size = 0;
		buffer->time_stamp = 0;
		fBuffers.AddItem(buffer);
	}
}

// _FreeBuffers
//
// Frees the allocated buffers.
void
AudioTrackReader::_FreeBuffers()
{
	if (fBuffers.CountItems() > 0) {
		delete[] (char*)_BufferAt(0)->data;
		for (int32 i = 0; Buffer* buffer = _BufferAt(i); i++)
			delete buffer;
		fBuffers.MakeEmpty();
	}
}

// _BufferAt
//
// Returns the buffer at index /index/.
AudioTrackReader::Buffer*
AudioTrackReader::_BufferAt(int32 index) const
{
	return (Buffer*)fBuffers.ItemAt(index);
}

// _FindBufferAtFrame
//
// If any buffer starts at offset /frame/, it is returned, NULL otherwise.
AudioTrackReader::Buffer*
AudioTrackReader::_FindBufferAtFrame(int64 frame) const
{
	Buffer* buffer = NULL;
	for (int32 i = 0;
		 ((buffer = _BufferAt(i))) && buffer->offset != frame;
		 i++);
	return buffer;
}

// _FindUnusedBuffer
//
// Returns the first unused buffer or NULL if all buffers are used.
AudioTrackReader::Buffer*
AudioTrackReader::_FindUnusedBuffer() const
{
	Buffer* buffer = NULL;
	for (int32 i = 0; ((buffer = _BufferAt(i))) && buffer->size != 0; i++);
	return buffer;
}

// _FindUsableBuffer
//
// Returns either an unused buffer or, if all buffers are used, the least
// recently used buffer.
// In every case a buffer is returned.
AudioTrackReader::Buffer*
AudioTrackReader::_FindUsableBuffer() const
{
	Buffer* result = _FindUnusedBuffer();
	if (!result) {
		// find the least recently used buffer.
		result = _BufferAt(0);
		for (int32 i = 1; Buffer* buffer = _BufferAt(i); i++) {
			if (buffer->time_stamp < result->time_stamp)
				result = buffer;
		}
	}
	return result;
}

// _FindUsableBufferFor
//
// In case there already exists a buffer that starts at position this
// one is returned. Otherwise the function returns either an unused
// buffer or, if all buffers are used, the least recently used buffer.
// In every case a buffer is returned.
AudioTrackReader::Buffer*
AudioTrackReader::_FindUsableBufferFor(int64 position) const
{
	Buffer* buffer = _FindBufferAtFrame(position);
	if (!buffer)
		buffer = _FindUsableBuffer();
	return buffer;
}

// _GetBuffersFor
//
// Adds pointers to all buffers to the list that contain data of the
// supplied interval.
void
AudioTrackReader::_GetBuffersFor(BList& buffers, int64 position,
								 int64 frames) const
{
	buffers.MakeEmpty();
	for (int32 i = 0; Buffer* buffer = _BufferAt(i); i++) {
		// Calculate the intersecting interval and add the buffer if it is
		// not empty.
		int32 startFrame = std::max(position, buffer->offset);
		int32 endFrame = std::min(position + frames,
			buffer->offset + buffer->size);
		if (startFrame < endFrame)
			buffers.AddItem(buffer);
	}
}

// _TouchBuffer
//
// Sets a buffer's time stamp to the current system time.
void
AudioTrackReader::_TouchBuffer(Buffer* buffer)
{
	buffer->time_stamp = system_time();
}

// _ReadBuffer
//
// Read a buffer from the current position (which is supplied in /position/)
// into /buffer/. The buffer's time stamp is set to the current system time.
status_t
AudioTrackReader::_ReadBuffer(Buffer* buffer, int64 position)
{
	return _ReadBuffer(buffer, position, system_time());
}

// _ReadBuffer
//
// Read a buffer from the current position (which is supplied in /position/)
// into /buffer/. The buffer's time stamp is set to the supplied time.
status_t
AudioTrackReader::_ReadBuffer(Buffer* buffer, int64 position, bigtime_t time)
{
	status_t error = fMediaTrack->ReadFrames(buffer->data, &buffer->size);
ldebug("  MediaTrack::Read(%p, %Ld): %ld\n", buffer->data, buffer->size,
error);
	buffer->offset = position;
	buffer->time_stamp = time;
	if (error != B_OK)
		buffer->size = 0;
	return error;
}

// _ReadCachedFrames
//
// Tries to read as much as possible data from the cache. The supplied
// buffer pointer as well as position and number of frames are adjusted
// accordingly. The used cache buffers are stamped with the current
// system time.
void
AudioTrackReader::_ReadCachedFrames(void*& dest, int64& pos, int64& frames)
{
	_ReadCachedFrames(dest, pos, frames, system_time());
}

// _ReadCachedFrames
//
// Tries to read as much as possible data from the cache. The supplied
// buffer pointer as well as position and number of frames are adjusted
// accordingly. The used cache buffers are stamped with the supplied
// time.
void
AudioTrackReader::_ReadCachedFrames(void*& dest, int64& pos, int64& frames,
									bigtime_t time)
{
	// Get a list of all cache buffers that contain data of the interval,
	// and sort it.
	BList buffers(10);
	_GetBuffersFor(buffers, pos, frames);
	buffers.SortItems(Buffer::CompareOffset);
	// Step forward through the list of cache buffers and try to read as
	// much data from the beginning as possible.
	for (int32 i = 0; Buffer* buffer = (Buffer*)buffers.ItemAt(i); i++) {
		if (buffer->offset <= pos && buffer->offset + buffer->size > pos) {
			// read from the beginning
			int64 size = std::min(frames, buffer->offset + buffer->size - pos);
			_CopyFrames(buffer->data, buffer->offset, dest, pos, pos, size);
			pos += size;
			frames -= size;
			dest = SkipFrames(dest, size);
		}
		buffer->time_stamp = time;
	}
	// Step backward through the list of cache buffers and try to read as
	// much data from the end as possible.
	for (int32 i = buffers.CountItems() - 1;
		 Buffer* buffer = (Buffer*)buffers.ItemAt(i);
		 i++) {
		if (buffer->offset < pos + frames
			&& buffer->offset + buffer->size >= pos + frames) {
			// read from the end
			int64 size = std::min(frames, pos + frames - buffer->offset);
			_CopyFrames(buffer->data, buffer->offset, dest, pos,
						pos + frames - size, size);
			frames -= size;
		}
	}
}

// _ReadUncachedFrames
//
// Reads /frames/ frames from /position/ into /buffer/. The frames are not
// read from the cache, but read frames are cached, if possible.
// New cache buffers are stamped with the system time.
// If an error occurs, the untouched part of the buffer is set to 0.
status_t
AudioTrackReader::_ReadUncachedFrames(void* buffer, int64 position,
									  int64 frames)
{
	return _ReadUncachedFrames(buffer, position, frames, system_time());
}

// _ReadUncachedFrames
//
// Reads /frames/ frames from /position/ into /buffer/. The frames are not
// read from the cache, but read frames are cached, if possible.
// New cache buffers are stamped with the supplied time.
// If an error occurs, the untouched part of the buffer is set to 0.
status_t
AudioTrackReader::_ReadUncachedFrames(void* buffer, int64 position,
									  int64 frames, bigtime_t time)
{
	status_t error = B_OK;
	// seek to the position
	int64 currentPos = position;
	if (frames > 0) {
//		error = fMediaTrack->SeekToFrame(&currentPos,
//										 B_MEDIA_SEEK_CLOSEST_BACKWARD);
		error = _SeekToKeyFrameBackward(currentPos);
ldebug("  seeked to position: %Ld\n", currentPos);
	}
	// read the frames
	while (error == B_OK && frames > 0) {
		Buffer* cacheBuffer = _FindUsableBufferFor(currentPos);
ldebug("  usable buffer found: %p\n", cacheBuffer);
		error = _ReadBuffer(cacheBuffer, currentPos, time);
		if (error == B_OK) {
			int64 size = std::min(position + frames,
				cacheBuffer->offset + cacheBuffer->size) - position;
			if (size > 0) {
				_CopyFrames(cacheBuffer, buffer, position, position, size);
				buffer = SkipFrames(buffer, size);
				position += size;
				frames -= size;
			}
			currentPos += cacheBuffer->size;
		}
	}
	// Ensure that all frames up to the next key frame are cached.
	// This avoids, that each read 
	if (error == B_OK) {
		int64 nextKeyFrame = currentPos;
//		if (fMediaTrack->FindKeyFrameForFrame(
//				&nextKeyFrame, B_MEDIA_SEEK_CLOSEST_FORWARD) == B_OK) {
		if (_FindKeyFrameForward(nextKeyFrame) == B_OK) {
			while (currentPos < nextKeyFrame) {
				// Check, if data at this position are cache.
				// If not read it.
				Buffer* cacheBuffer = _FindBufferAtFrame(currentPos);
				if (!cacheBuffer || cacheBuffer->size == 0) {
					cacheBuffer = _FindUsableBufferFor(currentPos);
					if (_ReadBuffer(cacheBuffer, currentPos, time) != B_OK)
						break;
				}
				if (cacheBuffer)
					currentPos += cacheBuffer->size;
			}
		}
	}
	// on error fill up the buffer with silence
	if (error != B_OK && frames > 0)
		ReadSilence(buffer, frames);
	return error;
}

// _FindKeyFrameForward
status_t
AudioTrackReader::_FindKeyFrameForward(int64& position)
{
	status_t error = B_OK;
// NOTE: the keyframe version confuses the Frauenhofer MP3 decoder,
// it works fine with the non-keyframe version, so let's hope this
// is the case for all other keyframe based BeOS codecs...
//	if (fHasKeyFrames) {
//		error = fMediaTrack->FindKeyFrameForFrame(
//			&position, B_MEDIA_SEEK_CLOSEST_FORWARD);
//	} else {
		int64 framesPerBuffer = _FramesPerBuffer();
		position += framesPerBuffer - 1;
		position = position % framesPerBuffer;
//	}
	return error;
}

// _FindKeyFrameBackward
status_t
AudioTrackReader::_FindKeyFrameBackward(int64& position)
{
	status_t error = B_OK;
	if (fHasKeyFrames) {
		error = fMediaTrack->FindKeyFrameForFrame(
			&position, B_MEDIA_SEEK_CLOSEST_BACKWARD);
	} else
		position -= position % _FramesPerBuffer();
	return error;
}

// _SeekToKeyFrameForward
status_t
AudioTrackReader::_SeekToKeyFrameForward(int64& position)
{
	if (position == fMediaTrack->CurrentFrame())
		return B_OK;

	status_t error = B_OK;
	if (fHasKeyFrames) {
int64 oldPosition = position;
		error = fMediaTrack->SeekToFrame(&position,
										 B_MEDIA_SEEK_CLOSEST_FORWARD);
ldebug("  seek to key frame forward: %Ld -> %Ld (%Ld)\n",
oldPosition, position, fMediaTrack->CurrentFrame());
	} else {
		_FindKeyFrameForward(position);
//if (position != fMediaTrack->CurrentFrame())
		error = fMediaTrack->SeekToFrame(&position);
	}
	return error;
}

// _SeekToKeyFrameBackward
status_t
AudioTrackReader::_SeekToKeyFrameBackward(int64& position)
{
	if (position == fMediaTrack->CurrentFrame())
		return B_OK;

	status_t error = B_OK;
	if (fHasKeyFrames) {
		int64 oldPosition = position;
		error = fMediaTrack->FindKeyFrameForFrame(&position,
			B_MEDIA_SEEK_CLOSEST_BACKWARD);
		if (error >= B_OK)
			error = fMediaTrack->SeekToFrame(&position, 0);
		if (error < B_OK) {
			position = fMediaTrack->CurrentFrame();
			if (fReportSeekError) {
				printf("  seek to key frame backward: %Ld -> %Ld (%Ld) - %s\n",
					oldPosition, position, fMediaTrack->CurrentFrame(),
					strerror(error));
				fReportSeekError = false;
			}
		} else {
			fReportSeekError = true;
		}
	} else {
		_FindKeyFrameBackward(position);
//if (position != fMediaTrack->CurrentFrame())
		error = fMediaTrack->SeekToFrame(&position);
	}
	return error;
}


/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaylistRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <Region.h>

#include "BBitmapBuffer.h"
#include "Painter.h"
#include "Playlist.h"
#include "RenderPlaylist.h"
#include "RenderPlaylistItem.h"
#include "VideoRenderer.h"

using std::nothrow;


class PlaylistRenderer::ChunkReaderSupport {
public:
	ChunkReaderSupport()
		:
		fLastPlaylistFrame(-1),
		fLastChunkProvider(NULL),
		fBuffer(NULL),
		fSize(0)
	{
		memset(&fChunkHeader, 0, sizeof(media_header));
	}

	status_t GetNextChunk(int32 frame, VideoRenderer* chunkProvider,
		const void*& buffer, size_t& size, bool& chunksComplete)
	{
printf("ChunkReaderSupport::GetNextChunk(%ld, %p)\n", frame, chunkProvider);
		// NOTE: frame needs to be offset to be local to the chunkProvider
		chunksComplete = false;

		if (fLastPlaylistFrame == frame - 1
			&& chunkProvider == fLastChunkProvider
			&& fBuffer != NULL) {
printf("  using buffer from previous invokation\n");
			// In this case, we have read the requested chunk in the previous
			// invokation.
			buffer = fBuffer;
			size = fSize;
			fBuffer = NULL;
			fSize = 0;
			return B_OK;
		}
		if (chunkProvider != fLastChunkProvider
			|| (fLastPlaylistFrame != frame - 1
				&& fLastPlaylistFrame != frame)) {
printf("  need to seek: %ld/%lld, fLastChunkProvider: %p, "
	"fLastPlaylistFrame: %ld\n", frame,
	chunkProvider->CurrentFrame(), fLastChunkProvider, fLastPlaylistFrame);
			// Seek the track, if it has changed and frame is indeed
			// a keyframe!
			int64 keyFrame = frame;
			if (chunkProvider->FindKeyFrameForFrame(&keyFrame,
					B_MEDIA_SEEK_CLOSEST_BACKWARD) != B_OK) {
printf("    FindKeyFrameForFrame() error\n");
				return B_ERROR;
			}
			if (keyFrame != frame) {
printf("    %ld is not a keyframe (%lld was last keyframe)\n", frame, keyFrame);
				return B_ERROR;
			}
			// NOTE: We cannot perform this check in the beginning, since that
			// doesn't guarantee that we have written the keyframe as the rest
			// of the frames in the group of pictures reference it.
			if (chunkProvider->CurrentFrame() != keyFrame) {
printf("    seek needs to be performed: %lld/%lld\n", keyFrame, chunkProvider->CurrentFrame());
				if (chunkProvider->SeekToFrame(&keyFrame, 0) != B_OK) {
					return B_ERROR;
				}
			}
			fLastChunkTime = chunkProvider->CurrentTime();
		}

		status_t ret = chunkProvider->ReadChunk(&fBuffer, &fSize,
			&fChunkHeader);
		if (ret == B_LAST_BUFFER_ERROR) {
printf("  B_LAST_BUFFER_ERROR\n");
			chunksComplete = true;
			fLastPlaylistFrame = frame;
			fLastChunkProvider = chunkProvider;
			return B_OK;
		}
		if (ret != B_OK) {
printf("  other error\n");
			return ret;
		}

		fLastChunkProvider = chunkProvider;
		fLastPlaylistFrame = frame;

		if (fChunkHeader.start_time != fLastChunkTime) {
			// We have read a chunk that belongs to the next frame already.
printf("  read chunk for next frame\n");
			fLastChunkTime = fChunkHeader.start_time;
			chunksComplete = true;
			return B_OK;
		}

		buffer = fBuffer;
		size = fSize;
		fBuffer = NULL;
		fSize = 0;

		return B_OK;
	}

	void Unset()
	{
printf("ChunkReaderSupport::Unset()\n");
		fLastChunkProvider = NULL;
		fLastPlaylistFrame = -1;
	}

	const media_header&	ChunkHeader() const
	{
		return fChunkHeader;
	}

private:
	int32			fLastPlaylistFrame;
	bigtime_t		fLastChunkTime;
	VideoRenderer*	fLastChunkProvider;

	const void*		fBuffer;
	size_t			fSize;
	media_header	fChunkHeader;
};

// #pragma mark - PlaylistRenderer

// constructor
PlaylistRenderer::PlaylistRenderer(Playlist* playlist, uint32 width,
		uint32 height, uint32 flags, color_space format)
	:
	fPlaylist(playlist),
	fPainter(),
	fRendererCache(),
	fCacheBitmap(new (nothrow) BBitmap(BRect(0.0, 0.0, width - 1, height - 1),
		format)),
	fFlags(flags),
	fPrintError(true),
	fLastChunk(NULL)
{
	if (!IsValid())
		return;

	BBitmapBuffer buffer(fCacheBitmap);
	fPainter.AttachToBuffer(&buffer);
}

// destructor
PlaylistRenderer::~PlaylistRenderer()
{
	delete fCacheBitmap;
	delete fLastChunk;
}

// IsValid
bool
PlaylistRenderer::IsValid() const
{
	return fCacheBitmap && fCacheBitmap->IsValid();
}

// RenderFrame
status_t
PlaylistRenderer::RenderFrame(int32 frame, bool& wasCached)
{
	wasCached = false;

	fRendererCache.DeleteOldRenderers();

	if (fPlaylist) {
		fPlaylist->SetCurrentFrame(frame);
		// temporary render playlist
		RenderPlaylist playlist(*fPlaylist, (double)frame,
			fCacheBitmap->ColorSpace(), &fRendererCache);

		// clear the buffer only where needed
		BRegion cleanBG(fPainter.Bounds());
		playlist.RemoveSolidRegion(&cleanBG, &fPainter, frame);
		fPainter.ClearBuffer(cleanBG);

		// render
		status_t ret = playlist.Generate(&fPainter, frame);
		if (ret < B_OK) {
			if (fPrintError) {
				printf("PlaylistRenderer::RenderFrame() - "
					"error rendering playlist: %s\n", strerror(ret));
				fPrintError = false;
			}
			fPainter.ClearBuffer();
		} else
			fPrintError = true;
		return B_OK;
	}

	return B_NO_INIT;
}

// GetNextVideoChunk
status_t
PlaylistRenderer::GetNextVideoChunk(int32 frame, const void*& buffer,
	size_t& size, bool& chunksComplete, const media_codec_info& codecInfo)
{
	if (fPlaylist == NULL)
		return B_NO_INIT;

	// This only works if a couple of conditions are met:
	// * There is only one video track,
	// * the project dimensions equal the track's video dimensions,
	// * the video is unmodified in any way (no transformation, no filters),
	// * the current frame is a key-frame OR all these conditions were true
	//   for the previous frame.

	// Iterate over the playlist items at the frame and check most of the
	// conditions
	fPlaylist->SetCurrentFrame(frame);
	// temporary render playlist
	RenderPlaylist playlist(*fPlaylist, (double)frame,
		fCacheBitmap->ColorSpace(), &fRendererCache);
	int32 count = playlist.CountItems();
	VideoRenderer* videoRenderer = NULL;
	RenderPlaylistItem* item = NULL;
	for (int32 i = 0; i < count; i++) {
		item = dynamic_cast<RenderPlaylistItem*>(playlist.ItemAtFast(i));
		if (item == NULL)
			continue;
		if (item->HasVideo()) {
			if (videoRenderer != NULL)
				return B_ERROR;
			videoRenderer = dynamic_cast<VideoRenderer*>(item->Renderer());
		}
	}

	if (videoRenderer == NULL)
		return B_ERROR;

	// TODO: Playlists don't currently support aspect ratio, so there is more
	// to this than the check below!
	if (videoRenderer->DisplayBounds() != fCacheBitmap->Bounds()) {
		fprintf(stderr, "PlaylistRenderer::GetNextVideoChunk() - "
			"item has wrong video size\n");
		return B_ERROR;
	}

	if (fLastChunk == NULL) {
		fLastChunk = new(std::nothrow) ChunkReaderSupport();
		if (fLastChunk == NULL)
			return B_NO_MEMORY;
	}

	// Check unmodified item (no transformation...)
	// NOTE: This whole Smart Rendering thing works for the top-level
	// playlist items only. If there are sub-playlists, there could be
	// a situation in which Smart Rendering is possible, but since we check
	// for a VideoRenderer in the top-level items only, it will not be
	// detected. Therefor we do not need to run the graphics state stack
	// until we hit our item, we can just check the properties directly.
	// TODO: If Smart Rendering items in sub-playlists is ever supported,
	// this would need to be changed of course.
	if (!item->Transformation().IsIdentity() || item->Alpha() != 1.0) {
		fprintf(stderr, "PlaylistRenderer::GetNextVideoChunk() - "
			"item has modifications\n");
		return B_ERROR;
	}

	// Check that the codec format is even the same...
	media_codec_info chunkCodecInfo;
	if (videoRenderer->GetCodecInfo(&chunkCodecInfo) != B_OK) {
		fprintf(stderr, "PlaylistRenderer::GetNextVideoChunk() - "
			"unable to retrieve codec info of item\n");
		return B_ERROR;
	}
	if (chunkCodecInfo.id != codecInfo.id
		|| chunkCodecInfo.sub_id != codecInfo.sub_id) {
		fprintf(stderr, "PlaylistRenderer::GetNextVideoChunk() - "
			"mismatching codecs: %ld/%ld <-> %ld/%ld\n",
			chunkCodecInfo.id, chunkCodecInfo.sub_id, codecInfo.id,
			codecInfo.sub_id);
		return B_ERROR;
	}

	status_t ret = fLastChunk->GetNextChunk(frame - item->StartFrame(),
		videoRenderer, buffer, size, chunksComplete);
	if (ret != B_OK)
		fLastChunk->Unset();
	return ret;
}

// ChunkHeader
const media_header&
PlaylistRenderer::ChunkHeader() const
{
	return fLastChunk->ChunkHeader();
}

// Bitmap
const BBitmap*
PlaylistRenderer::Bitmap() const
{
	return fCacheBitmap;
}


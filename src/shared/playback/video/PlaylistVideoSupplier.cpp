/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PlaylistVideoSupplier.h"

#include <stdio.h>

#include <MediaDefs.h>

#include "MediaRenderingBuffer.h"
#include "Painter.h"
#include "RenderPlaylist.h"
#include "RWLocker.h"

// debugging
//#include "Debug.h"
////#define ldebug	debug
//#define ldebug	nodebug
//
//struct id_data_pair {
//	registry_entry_id	id;
//	uint8*				data;
//};

// constructor
PlaylistVideoSupplier::PlaylistVideoSupplier(Playlist* list,
											 RWLocker* locker)
	: fPlaylist(NULL)
	, fPainter()
	, fLocker(locker)
	, fRendererCache()
{
	SetPlaylist(list);
}

// destructor
PlaylistVideoSupplier::~PlaylistVideoSupplier()
{
	SetPlaylist(NULL);
}

// FillBuffer
status_t
PlaylistVideoSupplier::FillBuffer(int64 frame, void* buffer,
								  const media_format* format,
								  bool& forceOrWasCached)
{
	bigtime_t now = system_time();

	fRendererCache.DeleteOldRenderers();

	// update Painter to point to this buffer
	MediaRenderingBuffer mediaBuffer(buffer, format);
	if (!fPainter.MemoryDestinationChanged(&mediaBuffer)) {
		fprintf(stderr, "PlaylistVideoSupplier: - "
						"unable to attach Painter to buffer!\n");
		return B_ERROR;
	}
	// clear buffer
	fPainter.ClearBuffer();

	AutoReadLocker locker(fLocker);
	if (fLocker && !locker.IsLocked()) {
		printf("PlaylistVideoSupplier: - readlocking failed!\n");
		return B_ERROR;
	}

	status_t ret = B_NO_INIT;
	if (fPlaylist) {
		// create a temporary clone of the playlist
		// so that we can keep the time holding the
		// read lock really short
//bigtime_t _now = system_time();
		RenderPlaylist temporaryList(*fPlaylist,
			(double)frame, format->u.raw_video.display.format,
			&fRendererCache);
//printf("cloning playlist: %lldµsecs\n", system_time() - _now);

		// NOTE: assuming that VideoProducer is never interlaced
		double playlistFrame = (double)frame * fPlaylist->VideoFrameRate()
			/ format->u.raw_video.field_rate;

		locker.Unlock();
		// since we had to clear the buffer anyways,
		// it doesn't matter wether the playlist is
		// empty at the frame... so always return B_OK
		temporaryList.Generate(&fPainter, playlistFrame);

		// flush cached buffer, if there is one
		fPainter.FlushCaches();

		ret = B_OK;
	} else {
		fPainter.ClearBuffer();
		fPainter.FlushCaches();
		ret = B_OK;
	}
	if (ret == B_OK) {
		fProcessingLatency = system_time() - now;
	}

	forceOrWasCached = false;
	return ret;
}

// SetPlaylist
void
PlaylistVideoSupplier::SetPlaylist(Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	Playlist* oldList = fPlaylist;

	fPlaylist = playlist;

	if (fPlaylist)
		fPlaylist->Acquire();

	if (oldList)
		oldList->Release();
}


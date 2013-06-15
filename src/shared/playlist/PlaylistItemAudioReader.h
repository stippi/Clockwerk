/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_ITEM_AUDIO_READER_H
#define PLAYLIST_ITEM_AUDIO_READER_H

#include "AudioReader.h"

class PlaylistItem;

#define MAX_GAIN_VALUES 128

class PlaylistItemAudioReader : public AudioReader {
 public:
								PlaylistItemAudioReader(PlaylistItem* item,
														AudioReader* source);
	virtual						~PlaylistItemAudioReader();

	virtual	status_t			Read(void* buffer, int64 pos, int64 frames);

	virtual	status_t			InitCheck() const;

 private:
			AudioReader*		fSource;
			PlaylistItem*		fItem;

			float				fCurrentGains[MAX_GAIN_VALUES];
			int32				fCurrentGainIndex;
			float				fLastGain;
};

#endif	// PLAYLIST_ITEM_AUDIO_READER_H

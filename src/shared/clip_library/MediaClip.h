/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef MEDIA_CLIP_H
#define MEDIA_CLIP_H

#include <MediaDefs.h>

#include "FileBasedClip.h"

class BMediaFile;
class BMediaTrack;

class MediaClip : public FileBasedClip {
public:
								MediaClip(const entry_ref* ref,
										  BMediaFile* file,
										  BMediaTrack* videoTrack,
										  BMediaTrack* audioTrack);
								MediaClip(const entry_ref* ref,
									const BMessage& archive);
	virtual						~MediaClip();

// TODO: Duration() needs to get a playlistFPS value!
// ... or return a time value

	// Clip interface
	virtual	uint64				Duration();
	virtual	uint64				MaxDuration();

	virtual	bool				HasVideo();

	virtual	bool				HasAudio();
	virtual	AudioReader*		CreateAudioReader();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	// FileBasedClip interface
	virtual	status_t			InitCheck();
		// TODO: there should be one more method
		// like "RecognizeFile()", so that
		// other FileBasedClip types are not
		// tried anymore

	static	Clip*				CreateClip(const entry_ref* ref,
									status_t& error);

	static	BRect				VideoBounds(const media_format& format);

protected:
	virtual	void				HandleReload();

			status_t			_StoreArchive();

	static	status_t			_GetMedia(const entry_ref* ref,
									BMediaFile*& file,
									BMediaTrack** _videoTrack,
									BMediaTrack** _audioTrack);

private:
			bool				fHasVideoTrack;
			bool				fHasAudioTrack;
			BRect				fBounds;
			uint64				fVideoFrameCount;
			uint64				fAudioFrameCount;
			float				fVideoFPS;
			float				fAudioFPS;
};

#endif // MEDIA_CLIP_H

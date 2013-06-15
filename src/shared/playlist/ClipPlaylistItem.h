/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_PLAYLIST_ITEM_H
#define CLIP_PLAYLIST_ITEM_H

#include "PlaylistItem.h"
#include "Observer.h"

class Clip;

class ClipPlaylistItem : public PlaylistItem,
						 public Observer {
 public:
								ClipPlaylistItem(::Clip* clip,
												 int64 startFrame = 0,
												 uint32 track = 0);
								ClipPlaylistItem(
									const ClipPlaylistItem& other,
									bool deep);
								ClipPlaylistItem(BMessage* archive);
	virtual						~ClipPlaylistItem();

	// BArchivable support
	__attribute__ ((visibility ("default")))
	static	BArchivable*		Instantiate(BMessage* archive);

	// PlaylistItem interface
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

	virtual	PlaylistItem*		Clone(bool deep) const;

	virtual	bool				HasAudio();
	virtual	AudioReader*		CreateAudioReader();

	virtual	bool				HasVideo();

	virtual	BRect				Bounds(BRect canvasBounds,
									   bool transformed = true);

	virtual	BString				Name() const;

	virtual	bool				MouseDown(BPoint where, uint32 buttons,
									BRect canvasBounds, double frame,
									PlaybackNavigator* navigator);

	virtual	uint64				MaxDuration() const;

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// ClipPlaylistItem
			void				SetClip(::Clip* clip);
			::Clip*				Clip() const
									{ return fClip; }

	static	uint64				DefaultDuration(uint64 duration);

 private:
								ClipPlaylistItem(
									const ClipPlaylistItem& other);

 			::Clip*				fClip;
};

#endif // CLIP_PLAYLIST_ITEM_H

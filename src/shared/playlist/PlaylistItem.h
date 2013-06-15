/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_ITEM_H
#define PLAYLIST_ITEM_H

#include <Archivable.h>

#include "AffineTransform.h"
#include "PropertyObject.h"
#include "Selectable.h"

class AudioReader;
class FloatProperty;
class NavigationInfo;
class PlaybackNavigator;
class Playlist;
class PropertyAnimator;
class ServerObjectManager;

//enum {
//	VIDEO_ITEM_TYPE	= 0x01,
//	AUDIO_ITEM_TYPE	= 0x02,
//	LIST_ITEM_TYPE	= 0x04,
//};

class PlaylistItem : public PropertyObject, public Selectable,
	public BArchivable {
public:
								PlaylistItem(int64 startFrame = 0,
											 uint64 duration = 0,
											 uint32 track = 0);
								PlaylistItem(const PlaylistItem& other,
											 bool deep);
								PlaylistItem(BMessage* archive);
	virtual						~PlaylistItem();

	// Selectable interface
	virtual	void				SelectedChanged();

	// BArchivable interface
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

	// PlaylistItem
	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

	virtual	PlaylistItem*		Clone(bool deep) const = 0;
//	virtual	bool				SetTo(const PlaylistItem* other);
//	virtual	uint32				Type() const = 0;

			void				SetParent(Playlist* parent);
			Playlist*			Parent() const
									{ return fParent; }

	virtual	void				SetCurrentFrame(double frame);

	virtual	bool				HasVideo() = 0;
	virtual	bool				HasAudio() = 0;
	virtual	AudioReader*		CreateAudioReader() = 0;

			void				SetVideoMuted(bool muted);
			bool				IsVideoMuted() const
									{ return fVideoMuted; }

			void				SetAudioMuted(bool muted);
			bool				IsAudioMuted() const
									{ return fAudioMuted; }

	virtual	BRect				Bounds(BRect canvasBounds,
									   bool transformed = true) = 0;

	virtual BString				Name() const = 0;

	virtual	bool				MouseDown(BPoint where, uint32 buttons,
									BRect canvasBounds, double frame,
									PlaybackNavigator* navigator);

			void				SetNavigationInfo(const ::NavigationInfo* info);
			::NavigationInfo*	NavigationInfo() const
									{ return fNavigationInfo; }


			void				SetStartFrame(int64 startFrame);
			int64				StartFrame() const
									{ return fStartFrame + fClipOffset; }

			void				SetDuration(uint64 duration);
			uint64				Duration() const
									{ return fDuration - fClipOffset; }
	virtual	uint64				MaxDuration() const;
			bool				HasMaxDuration() const;

			void				SetClipOffset(uint64 offset);
			uint64				ClipOffset() const
									{ return fClipOffset; }

			void				SetTrack(uint32 track);
			uint32				Track() const
									{ return fTrack; }

			int64				EndFrame() const
									{ return fStartFrame + fDuration - 1; }

	virtual	void				ConvertFrameToLocal(int64& frame) const;

			float				VideoFramesPerSecond() const;

			AffineTransform		Transformation() const;
			float				Alpha() const;

			PropertyAnimator*	AlphaAnimator() const;

			// TODO: remove? (access via PropertyObject)
			FloatProperty*		PivotX() const
									{ return fPivotX; }
			FloatProperty*		PivotY() const
									{ return fPivotY; }
			FloatProperty*		TranslationX() const
									{ return fTranslationX; }
			FloatProperty*		TranslationY() const
									{ return fTranslationY; }
			FloatProperty*		Rotation() const
									{ return fRotation; }
			FloatProperty*		ScaleX() const
									{ return fScaleX; }
			FloatProperty*		ScaleY() const
									{ return fScaleY; }

 protected:
								PlaylistItem(const PlaylistItem& other);

 private:
			void				_CreateProperties();

			Playlist*			fParent;

			int64				fStartFrame;
			uint64				fDuration;
			uint64				fClipOffset;

			uint32				fTrack;

			// TODO: maybe put somewhere else
			// (should there be a VideoPlaylistItem subclass?)
			FloatProperty*		fAlpha;

			FloatProperty*		fPivotX;
			FloatProperty*		fPivotY;
			FloatProperty*		fTranslationX;
			FloatProperty*		fTranslationY;
			FloatProperty*		fRotation;
			FloatProperty*		fScaleX;
			FloatProperty*		fScaleY;

			bool				fVideoMuted;
			bool				fAudioMuted;

			::NavigationInfo*	fNavigationInfo;
};

#endif // PLAYLIST_ITEM_H

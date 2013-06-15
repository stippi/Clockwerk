/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <List.h>
#include <Point.h>
#include <String.h>

#include "Clip.h"

class DurationProperty;
class PlaybackNavigator;
class PlaylistItem;
class PlaylistObserver;
class TrackProperties;

class Playlist : public Clip {
public:
								Playlist();
								Playlist(const Playlist& other, bool deep);
	virtual						~Playlist();

	// Selectable
	virtual	void				SelectedChanged();

	// ServerObject interface
	virtual	status_t			SetTo(const ServerObject* other);
	virtual	bool				IsMetaDataOnly() const;
	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

// TODO: implement loading/saving through this interface
//	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	virtual	status_t			Unarchive(const BMessage* from);

	// Clip interface
	virtual	uint64				Duration();
	virtual	uint64				MaxDuration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	virtual	uint32				ChangeToken() const
									{ return fChangeToken; }
		// currently used to indicate changes of
		// content so that a ClipRenderer can update itself

	// TODO: move to Clip or maybe ClipPlaylistItem only?
	virtual	bool				MouseDown(BPoint where, uint32 buttons,
									BRect canvasBounds, double currentFrame,
									PlaybackNavigator* navigator);


	// Playlist
	virtual	void				ValidateItemLayout();

			void				AddListObserver(PlaylistObserver* observer);
			void				RemoveListObserver(PlaylistObserver* observer);
			void				StartNotificationBlock();
			void				FinishNotificationBlock();
			bool				IsInNotificationBlock() const
									{ return fNotificationBlocks > 0; }

			Playlist*			Clone(bool deep) const;

			void				SetCurrentFrame(double frame);

			float				VideoFrameRate() const
									{ return 25.0; }
			int32				Width() const;
			int32				Height() const;

	// list manipulation
			bool				AddItem(PlaylistItem* item);
			bool				AddItem(PlaylistItem* item,
										int32 index);
			PlaylistItem*		RemoveItem(int32 index);
			bool				RemoveItem(PlaylistItem* item);

			PlaylistItem*		ItemAt(int32 index) const;
			PlaylistItem*		ItemAtFast(int32 index) const;

			bool				HasItem(PlaylistItem* item) const;
			int32				IndexOf(PlaylistItem* item) const;

			int32				CountItems() const;

			void				MakeEmpty();

	// track manipulation
			bool				SetTrackProperties(const TrackProperties& properties);
			bool				ClearTrackProperties(uint32 track);

			TrackProperties*	PropertiesForTrack(uint32 track) const;
			TrackProperties*	TrackPropertiesAt(int32 index) const;
			TrackProperties*	TrackPropertiesAtFast(int32 index) const;

			int32				CountTrackProperties() const;

			void				SetSoloTrack(int32 track);
			int32				SoloTrack() const
									{ return fSoloTrack; }

			bool				IsTrackEnabled(uint32 track) const;

			void				MoveTrack(uint32 oldIndex, uint32 newIndex);
			void				InsertTrack(uint32 track);
			void				RemoveTrack(uint32 track);

	// useful stuff
			void				MakeRoom(int64 fromFrame,
										 int64 toFrame,
										 uint32 track,
										 PlaylistItem* ignoreItem,
										 int64* effectedRangeStart,
										 int64* pushedFrames);

			void				MoveItems(int64 startFrame,
										  int64 frames,
										  uint32 track,
										  PlaylistItem* ignoreItem);

			uint32				MaxTrack() const;

			void				ItemsChanged();

			void				GetFrameBounds(int64* firstFrame,
											   int64* lastFrame) const;

			void				PrintToStream() const;

 protected:
								Playlist(const char* type);

			bool				AddTrackProperties(TrackProperties* properties);

			void				SortItems(
									int (*cmp)(const void*, const void*));

	virtual	void				_ItemsChanged();

 private:
								Playlist(const Playlist& other);

			void				_SetDuration(uint64 duration);
			void				_SetMaxTrack(uint32 maxTrack);
			int32				_IndexForTrackProperties(uint32 track,
														 bool* exists) const;

			void				_NotifyItemAdded(PlaylistItem* item,
												 int32 index);
			void				_NotifyItemRemoved(PlaylistItem* item);
			void				_NotifyDurationChanged(uint64 duration);
			void				_NotifyMaxTrackChanged(uint32 maxTrack);
			void				_NotifyNotificationBlockStarted();
			void				_NotifyTrackPropertiesChanged(uint32 track);
			void				_NotifyTrackMoved(uint32 oldIndex,
												  uint32 newIndex);
			void				_NotifyTrackInserted(uint32 track);
			void				_NotifyTrackRemoved(uint32 track);
			void				_NotifyNotificationBlockFinished();

 			BList				fItems;

			BList				fTrackProperties;
			int32				fSoloTrack;

			BList				fObservers;
			int32				fNotificationBlocks;

			uint64				fDuration;
			uint32				fMaxTrack;
									// stores the index of the last track
									// with an item on it

			uint32				fChangeToken;

			DurationProperty*	fDurationProperty;
};

class PlaylistNotificationBlock {
 public:
	PlaylistNotificationBlock(Playlist* playlist)
		: fPlaylist(playlist)
	{
		fPlaylist->StartNotificationBlock();
	}
	~PlaylistNotificationBlock()
	{
		fPlaylist->FinishNotificationBlock();
	}
 private:
	Playlist* fPlaylist;
};

#endif // PLAYLIST_H

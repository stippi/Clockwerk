/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_GROUP_H
#define CLIP_GROUP_H

#include <View.h>

#include "ClipListView.h"

class BCheckBox;
class BMenu;
class BMenuField;
class BTextControl;

enum {
	CLIP_TYPE_ALL			= 0,

	CLIP_TYPE_ALL_FOR_UPLOAD,
	CLIP_TYPE_ALL_NEW,

	CLIP_TYPE_BITMAP,
	CLIP_TYPE_CLOCK,
	CLIP_TYPE_COLOR,
	CLIP_TYPE_MEDIA,
	CLIP_TYPE_PLAYLIST,
	CLIP_TYPE_TABLE,
	CLIP_TYPE_TICKER,
	CLIP_TYPE_TEXT,
	CLIP_TYPE_TIMER,
	CLIP_TYPE_WEATHER,

	CLIP_TYPE_EXECUTE,
};

class ClipGroup : public BView {
 public:
								ClipGroup(ClipListView* listView);
	virtual						~ClipGroup();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				MessageReceived(BMessage* message);

	// ClipGroup
			ClipListView*		ListView() const
									{ return fClipListView; }

			// which clips the listview shoud show:
			void				SetClipType(int32 type);
			int32				ClipType() const;
			void				SetPlaylistClipsOnly(bool playlistOnly);
			bool				PlaylistClipsOnly() const;
			void				SetNameContainsOnly(bool nameContainsOnly);
			bool				NameContainsOnly() const;
			void				SetNameContainsString(const char* string);
			const char*			NameContainsString() const;

			void				MakeSureClipShows(Clip* clip);

 private:
			BMenu*				_CreateTypeMenu() const;
			void				_SelectClipTypeItem(int32 type);
			ClipListView::ItemSorter* _SorterForType(int32 type);
			uint32				_TypeForTypeString(const BString& type) const;
			uint32				_TypeForClip(const Clip* clip) const;
			void				_SetFilterByNameEnabled(bool enabled);
			const char*			_NameContainsFilterString() const;

			ClipListView*		fClipListView;
			BMenuField*			fClipTypeMF;
			BMenu*				fClipTypeM;
			BCheckBox*			fPlaylistClipsOnlyCB;
			BCheckBox*			fFilterByNameCB;
			BTextControl*		fNameContainsTC;

			BRect				fPreviousBounds;
};

#endif // CLIP_GROUP_H

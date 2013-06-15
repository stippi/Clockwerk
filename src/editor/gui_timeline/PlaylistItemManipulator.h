/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYLIST_ITEM_MANIPULATOR_H
#define PLAYLIST_ITEM_MANIPULATOR_H

#include <String.h>

#include "Manipulator.h"

class PlaylistItem;
class TimelineView;

class PlaylistItemManipulator : public Manipulator {
 public:
								PlaylistItemManipulator(PlaylistItem* item);
	virtual						~PlaylistItemManipulator();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Manipulator interface
	virtual	bool				DoubleClicked(BPoint where);

	virtual	BRect				Bounds();

	virtual	void				AttachedToView(StateView* view);
	virtual	void				DetachedFromView(StateView* view);

	virtual	void				RebuildCachedData();

	// PlaylistItemManipulator
			PlaylistItem*		Item() const
									{ return fItem; }

 protected:
			BRect				_ComputeFrameFor(PlaylistItem* item) const;

			PlaylistItem*		fItem;
			BRect				fItemFrame;

			int64				fCachedStartFrame;
			int64				fCachedEndFrame;
			int32				fCachedTrack;
			bool				fCachedSelected;
			bool				fCachedMuted;
			BString				fCachedName;

			TimelineView*		fView;
};

#endif // PLAYLIST_ITEM_MANIPULATOR_H

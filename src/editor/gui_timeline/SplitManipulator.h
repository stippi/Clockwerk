/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SPLIT_MANIPULATOR_H
#define SPLIT_MANIPULATOR_H

#include "PlaylistItemManipulator.h"
#include "SnapFrameList.h"

// The intention is to have a "split" item frame
// where the upper half of the item is manipulated in
// a subclass of SplitManipulator, and the lower
// half is used to display a selected item property
// which is manipulated by a PropertyManipulator. This
// in turn is controlled in this item.

class PropertyManipulator;

class SplitManipulator : public PlaylistItemManipulator {
public:
								SplitManipulator(PlaylistItem* item);
	virtual						~SplitManipulator();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DisplayPopupMenu(BPoint where);

	virtual	void				RebuildCachedData();

	// SplitManipulator
	virtual	void				ToolDraw(BView* into, BRect itemFrame);
	virtual	bool				ToolMouseDown(BPoint where) = 0;
	virtual	void				ToolMouseMoved(BPoint where) = 0;
	virtual	Command*			ToolMouseUp() = 0;
	virtual	bool				ToolMouseOver(BPoint where) = 0;
	virtual	bool				ToolIsActive() = 0;

protected:
	friend class PropertyManipulator;

			BRect				_UpperFrame() const;

			PlaylistItem*		_Item() const
									{ return fItem; }
			TimelineView*		_View() const
									{ return fView; }
			BRect				_ItemFrame() const
									{ return fItemFrame; }

			PropertyManipulator*	fPropertyManipulator;
			bool				fTrackingProperty;

			SnapFrameList		fSnapFrames;
};

#endif // SPLIT_MANIPULATOR_H

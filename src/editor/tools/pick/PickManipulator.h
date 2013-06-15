/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PICK_MANIPULATOR_H
#define PICK_MANIPULATOR_H

#include "SplitManipulator.h"
#include "SnapFrameList.h"

class TrackingCommand;

class PickManipulator : public SplitManipulator {
 public:
								PickManipulator(PlaylistItem* item);
	virtual						~PickManipulator();

	// SplitManipulator interface
	virtual	void				ToolDraw(BView* into, BRect itemFrame);
	virtual	bool				ToolMouseDown(BPoint where);
	virtual	void				ToolMouseMoved(BPoint where);
	virtual	Command*			ToolMouseUp();
	virtual	bool				ToolMouseOver(BPoint where);
	virtual	bool				ToolIsActive();

 private:
			uint32				_TrackModeFor(BPoint where) const;
			void				_SetTrackMode(uint32 mode);
			void				_SetTracking(uint32 mode,
											 int64 startDragFrame);

			TrackingCommand*	fCommand;
			uint32				fTrackMode;
};

#endif // PICK_MANIPULATOR_H

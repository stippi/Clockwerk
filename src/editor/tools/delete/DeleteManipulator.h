/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DELETE_MANIPULATOR_H
#define DELETE_MANIPULATOR_H

#include "SplitManipulator.h"
#include "SnapFrameList.h"

class DeleteManipulator : public SplitManipulator {
 public:
								DeleteManipulator(PlaylistItem* item);
	virtual						~DeleteManipulator();

	// SplitManipulator interface
	virtual	bool				ToolMouseDown(BPoint where);
	virtual	void				ToolMouseMoved(BPoint where);
	virtual	Command*			ToolMouseUp();
	virtual	bool				ToolMouseOver(BPoint where);
	virtual	bool				ToolIsActive();

 private:
			void				_SetMouseInside(bool inside);
			bool				fMouseInsideItem;
};

#endif // DELETE_MANIPULATOR_H

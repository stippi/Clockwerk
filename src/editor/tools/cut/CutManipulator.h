/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CUT_MANIPULATOR_H
#define CUT_MANIPULATOR_H

#include "SplitManipulator.h"
#include "SnapFrameList.h"

class CutCommand;

class CutManipulator : public SplitManipulator {
 public:
								CutManipulator(PlaylistItem* item);
	virtual						~CutManipulator();

	// SplitManipulator interface
	virtual	bool				ToolMouseDown(BPoint where);
	virtual	void				ToolMouseMoved(BPoint where);
	virtual	Command*			ToolMouseUp();
	virtual	bool				ToolMouseOver(BPoint where);
	virtual	bool				ToolIsActive();

 private:
			CutCommand*			fCommand;
};

#endif // CUT_MANIPULATOR_H

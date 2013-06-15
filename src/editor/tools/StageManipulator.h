/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef STAGE_MANIPULATOR_H
#define STAGE_MANIPULATOR_H

#include "Manipulator.h"

class StageManipulator : public Manipulator {
 public:
								StageManipulator(Observable* object);
	virtual						~StageManipulator();

	virtual	void				SetCurrentFrame(int64 frame);
};

#endif // STAGE_MANIPULATOR_H

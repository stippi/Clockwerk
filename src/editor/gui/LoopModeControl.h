/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOOP_MODE_CONTROL_H
#define LOOP_MODE_CONTROL_H

#include <MenuField.h>

#include "Observer.h"

class LoopMode;

class LoopModeControl : public BMenuField, public Observer {
 public:
								LoopModeControl();
	virtual						~LoopModeControl();

	// BMenuField interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// LoopModeControl
			void				SetLoopMode(LoopMode* mode);

 private:
			BMenu*				_CreateMenu();

			LoopMode*			fLoopMode;
			int32				fLoopModeCache;

			BMenuItem*			fAllItem;
			BMenuItem*			fRangeItem;
			BMenuItem*			fVisibleItem;
};

#endif // LOOP_MODE_CONTROL_H

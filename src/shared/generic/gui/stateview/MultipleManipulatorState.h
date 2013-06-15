/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MULTIPLE_MANIPULATOR_STATE_H
#define MULTIPLE_MANIPULATOR_STATE_H

#include <List.h>

#include "ViewState.h"

class Manipulator;

class MultipleManipulatorState : public ViewState {
 public:
								MultipleManipulatorState(StateView* view);
	virtual						~MultipleManipulatorState();

	// ViewState interface
	virtual	void				Init();
	virtual	void				Cleanup();

	virtual	void				Draw(BView* into, BRect updateRect);
	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	virtual	void				MouseDown(BPoint where,
										  uint32 buttons,
										  uint32 clicks);

	virtual	void				MouseMoved(BPoint where,
										   uint32 transit,
										   const BMessage* dragMessage);
	virtual	Command*			MouseUp();

	virtual	void				ModifiersChanged(uint32 modifiers);

	virtual	bool				HandleKeyDown(
									const StateView::KeyEvent& event,
									Command** _command);
	virtual	bool				HandleKeyUp(
									const StateView::KeyEvent& event,
									Command** _command);

	virtual	bool				UpdateCursor();
	virtual	BRect				Bounds() const;

	// MultipleManipulatorState
			bool				AddManipulator(Manipulator* manipulator);
			Manipulator*		RemoveManipulator(int32 index);
			void				DeleteManipulators();

			int32				CountManipulators() const;
			Manipulator*		ManipulatorAt(int32 index) const;
			Manipulator*		ManipulatorAtFast(int32 index) const;

			bool				HandlesAllKeyEvents() const;

 private:
			BList				fManipulators;
			Manipulator*		fCurrentManipulator;
			Manipulator*		fPreviousManipulator;
			Manipulator*		fLastMouseMovedManipulator;
};

#endif // MULTIPLE_MANIPULATOR_STATE_H

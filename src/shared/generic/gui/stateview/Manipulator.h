/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef MANIPULATOR_H
#define MANIPULATOR_H

#include <Rect.h>

#include "Observer.h"
#include "StateView.h"

class Command;
class StateView;

class Manipulator : public Observer {
 public:
								Manipulator(Observable* object);
	virtual						~Manipulator();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);
	virtual	bool				DisplayPopupMenu(BPoint where);

	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	virtual	void				ModifiersChanged(uint32 modifiers);
	virtual	bool				HandleKeyDown(
									const StateView::KeyEvent& event,
									Command** _command);
	virtual	bool				HandleKeyUp(
									const StateView::KeyEvent& event,
									Command** _command);

	virtual	bool				HandlesAllKeyEvents() const;

	virtual	bool				UpdateCursor();

	virtual	BRect				Bounds() = 0;
		// the area that the manipulator is
		// occupying in the "parent" view
	virtual	BRect				TrackingBounds(BView* withinView);
		// the area within "view" in which the
		// Manipulator wants to receive MouseOver()
		// events

			void				AttachToView(StateView* view);
			void				DetachFromView(StateView* view);

	virtual	void				AttachedToView(StateView* view);
	virtual	void				DetachedFromView(StateView* view);

	virtual	void				RebuildCachedData();
		// tells the manipulator that it should rebuild all cached data

			void				Invalidate();
			void				Invalidate(const BRect& frame);

			void				TriggerUpdate();
		// triggers an update in the view containing the Manipulator

 protected:
			Observable*			fManipulatedObject;
			StateView*			fView;
};

#endif // MANIPULATOR_H

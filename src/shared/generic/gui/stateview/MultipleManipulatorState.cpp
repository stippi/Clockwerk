/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MultipleManipulatorState.h"

#include <stdio.h>

#include "Manipulator.h"
#include "StateView.h"

// constructor
MultipleManipulatorState::MultipleManipulatorState(StateView* view)
	: ViewState(view),
	  fManipulators(24),
	  fCurrentManipulator(NULL),
	  fPreviousManipulator(NULL)
{
}

// destructor
MultipleManipulatorState::~MultipleManipulatorState()
{
	DeleteManipulators();
}

// #pragma mark -

// Init
void
MultipleManipulatorState::Init()
{
}

// Cleanup
void
MultipleManipulatorState::Cleanup()
{
}

// #pragma mark -

// Draw
void
MultipleManipulatorState::Draw(BView* into, BRect updateRect)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->Bounds().Intersects(updateRect)) {
			into->PushState();
			manipulator->Draw(into, updateRect);
			into->PopState();
		}
	}
}

// MessageReceived
bool
MultipleManipulatorState::MessageReceived(BMessage* message,
										  Command** _command)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->MessageReceived(message, _command))
			return true;
	}
	return false;
}

// #pragma mark -

// MouseDown
void
MultipleManipulatorState::MouseDown(BPoint where, uint32 buttons, uint32 clicks)
{
	bool rightClick = buttons & B_SECONDARY_MOUSE_BUTTON;
	bool doubleClick = false;
	if (clicks == 2
		&& fPreviousManipulator
		&& fManipulators.HasItem(fPreviousManipulator)) {
		// valid double click (onto the same, still existing manipulator)
		if (fPreviousManipulator->TrackingBounds(fView).Contains(where)
			&& fPreviousManipulator->DoubleClicked(where)) {
			// TODO: eat the click here or wait for MouseUp?
			fPreviousManipulator = NULL;
			doubleClick = true;
		}
	}

	if (!doubleClick) {
		int32 count = fManipulators.CountItems();
		for (int32 i = count - 1; i >= 0; i--) {
			Manipulator* manipulator =
				(Manipulator*)fManipulators.ItemAtFast(i);
			if (!manipulator->TrackingBounds(fView).Contains(where))
				continue;
			if ((rightClick && manipulator->DisplayPopupMenu(where))
				|| manipulator->MouseDown(where)) {
				fCurrentManipulator = manipulator;
				break;
			}
		}
		if (!fCurrentManipulator)
			fView->NothingClicked(where, buttons, clicks);
	}

	fView->SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

// MouseMoved
void
MultipleManipulatorState::MouseMoved(BPoint where, uint32 transit,
									 const BMessage* dragMessage)
{
	if (fCurrentManipulator) {
		// the mouse is currently pressed
		fCurrentManipulator->MouseMoved(where);
		fLastMouseMovedManipulator = fCurrentManipulator;
	} else {
		// the mouse is currently NOT pressed

		fLastMouseMovedManipulator = NULL;
		// call MouseOver on all manipulators
		// until one feels responsible
		int32 count = fManipulators.CountItems();
		for (int32 i = 0; i < count; i++) {
			Manipulator* manipulator =
				(Manipulator*)fManipulators.ItemAtFast(i);
			if (manipulator->TrackingBounds(fView).Contains(where)
				&& manipulator->MouseOver(where)) {
				fLastMouseMovedManipulator = manipulator;
				break;
			}
		}
	}
}

// MouseUp
Command*
MultipleManipulatorState::MouseUp()
{
	Command* command = NULL;
	if (fCurrentManipulator) {
		command = fCurrentManipulator->MouseUp();
		fPreviousManipulator = fCurrentManipulator;
		fCurrentManipulator = NULL;
	}
	return command;
}

// #pragma mark -

// ModifiersChanged
void
MultipleManipulatorState::ModifiersChanged(uint32 modifiers)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		manipulator->ModifiersChanged(modifiers);
	}
}

// HandleKeyDown
bool
MultipleManipulatorState::HandleKeyDown(const StateView::KeyEvent& event,
										Command** _command)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->HandleKeyDown(event, _command))
			return true;
	}
	return false;
}

// HandleKeyUp
bool
MultipleManipulatorState::HandleKeyUp(const StateView::KeyEvent& event,
									  Command** _command)
{
	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator =
			(Manipulator*)fManipulators.ItemAtFast(i);
		if (manipulator->HandleKeyUp(event, _command))
			return true;
	}
	return false;
}

// UpdateCursor
bool
MultipleManipulatorState::UpdateCursor()
{
	if (fLastMouseMovedManipulator && fManipulators.HasItem(fLastMouseMovedManipulator))
		return fLastMouseMovedManipulator->UpdateCursor();
	return false;
}


// Bounds
BRect
MultipleManipulatorState::Bounds() const
{
	BRect bounds(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
	int32 count = CountManipulators();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator = ManipulatorAtFast(i);
		bounds = bounds | manipulator->Bounds();
		
	}
	return bounds;
}

// #pragma mark -

// AddManipulator
bool
MultipleManipulatorState::AddManipulator(Manipulator* manipulator)
{
	if (!manipulator)
		return false;

	if (fManipulators.AddItem((void*)manipulator)) {
		manipulator->AttachToView(fView);
		fView->ViewStateBoundsChanged();
		fView->Invalidate(manipulator->Bounds());
		return true;
	}
	return false;
}

// RemoveManipulator
Manipulator*
MultipleManipulatorState::RemoveManipulator(int32 index)
{
	Manipulator* manipulator = (Manipulator*)fManipulators.RemoveItem(index);
	if (manipulator) {
		fView->ViewStateBoundsChanged();
		fView->Invalidate(manipulator->Bounds());
		manipulator->DetachFromView(fView);
	}

	if (manipulator == fCurrentManipulator)
		fCurrentManipulator = NULL;
	if (manipulator == fPreviousManipulator)
		fPreviousManipulator = NULL;
	if (manipulator == fLastMouseMovedManipulator)
		fLastMouseMovedManipulator = NULL;

	return manipulator;
}

// DeleteManipulators
void
MultipleManipulatorState::DeleteManipulators()
{
	BRect dirty(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);

	int32 count = fManipulators.CountItems();
	for (int32 i = 0; i < count; i++) {
		Manipulator* m = (Manipulator*)fManipulators.ItemAtFast(i);
		dirty = dirty | m->Bounds();
		m->DetachFromView(fView);
		delete m;
	}
	fManipulators.MakeEmpty();
	fCurrentManipulator = NULL;
	fPreviousManipulator = NULL;
	fLastMouseMovedManipulator = NULL;

	fView->ViewStateBoundsChanged();
	fView->Invalidate(dirty);
}

// CountManipulators
int32
MultipleManipulatorState::CountManipulators() const
{
	return fManipulators.CountItems();
}

// ManipulatorAt
Manipulator*
MultipleManipulatorState::ManipulatorAt(int32 index) const
{
	return (Manipulator*)fManipulators.ItemAt(index);
}

// ManipulatorAtFast
Manipulator*
MultipleManipulatorState::ManipulatorAtFast(int32 index) const
{
	return (Manipulator*)fManipulators.ItemAtFast(index);
}

// ManipulatorAtFast
bool
MultipleManipulatorState::HandlesAllKeyEvents() const
{
	int32 count = CountManipulators();
	for (int32 i = 0; i < count; i++) {
		Manipulator* manipulator = ManipulatorAtFast(i);
		if (manipulator->HandlesAllKeyEvents())
			return true;
	}
	return false;
}



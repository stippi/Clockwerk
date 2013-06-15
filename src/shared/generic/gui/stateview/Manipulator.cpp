/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Manipulator.h"

#include <View.h>

#include "Observable.h"

// constructor
Manipulator::Manipulator(Observable* object)
	: Observer(),
	  fManipulatedObject(object),
	  fView(NULL)
{
	if (fManipulatedObject)
		fManipulatedObject->AddObserver(this);
}

// destructor
Manipulator::~Manipulator()
{
	if (fManipulatedObject)
		fManipulatedObject->RemoveObserver(this);
}

// #pragma mark -

// Draw
void
Manipulator::Draw(BView* into, BRect updateRect)
{
}

// MouseDown
bool
Manipulator::MouseDown(BPoint where)
{
	return false;
}

// MouseMoved
void
Manipulator::MouseMoved(BPoint where)
{
}

// MouseUp
Command*
Manipulator::MouseUp()
{
	return NULL;
}

// MouseOver
bool
Manipulator::MouseOver(BPoint where)
{
	return false;
}

// DoubleClicked
bool
Manipulator::DoubleClicked(BPoint where)
{
	return false;
}

// DisplayPopupMenu
bool
Manipulator::DisplayPopupMenu(BPoint where)
{
	return false;
}

// #pragma mark -

bool
Manipulator::MessageReceived(BMessage* message, Command** _command)
{
	return false;
}

// #pragma mark -

// ModifiersChanged
void
Manipulator::ModifiersChanged(uint32 modifiers)
{
}

// HandleKeyDown
bool
Manipulator::HandleKeyDown(const StateView::KeyEvent& event,
						   Command** _command)
{
	return false;
}

// HandleKeyUp
bool
Manipulator::HandleKeyUp(const StateView::KeyEvent& event,
						 Command** _command)
{
	return false;
}

// HandlesAllKeyEvents
bool
Manipulator::HandlesAllKeyEvents() const
{
	return false;
}

// UpdateCursor
bool
Manipulator::UpdateCursor()
{
	return false;
}

// #pragma mark -

// TrackingBounds
BRect
Manipulator::TrackingBounds(BView* withinView)
{
	return Bounds();
}

// AttachToView
void
Manipulator::AttachToView(StateView* view)
{
	fView = view;
	AttachedToView(view);
}

// DetachFromView
void
Manipulator::DetachFromView(StateView* view)
{
	fView = NULL;
	DetachedFromView(view);
}

// AttachedToView
void
Manipulator::AttachedToView(StateView* view)
{
}

// DetachedFromView
void
Manipulator::DetachedFromView(StateView* view)
{
}

// RebuildCachedData
void
Manipulator::RebuildCachedData()
{
}

// Invalidate
void
Manipulator::Invalidate()
{
	Invalidate(Bounds());
}

// Invalidate
void
Manipulator::Invalidate(const BRect& frame)
{
	if (fView)
		fView->Invalidate(frame);
}

// TriggerUpdate
void
Manipulator::TriggerUpdate()
{
	if (StateView* view = dynamic_cast<StateView*>(fView))
		view->TriggerUpdate();
}

/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DropAnticipationState.h"

#include <stdio.h>

#include <Window.h>

#include "StateView.h"

// constructor
DropAnticipationState::DropAnticipationState(StateView* view)
	: ViewState(view),
	  fDragMessageClone((uint32)0),
	  fDropAnticipationRect(0, 0, -1, -1)
{
}

// destructor
DropAnticipationState::~DropAnticipationState()
{
}

// #pragma mark -

// Init
void
DropAnticipationState::Init()
{
}

// Cleanup
void
DropAnticipationState::Cleanup()
{
	_UnsetMessage();
	RemoveDropAnticipationRect();
}

// #pragma mark -

// Draw
void
DropAnticipationState::Draw(BView* into, BRect updateRect)
{
	if (!fDropAnticipationRect.IsValid())
		return;

	into->SetHighColor(255, 0, 0, 255);
	into->SetDrawingMode(B_OP_COPY);
	into->StrokeRect(fDropAnticipationRect);
}

// MessageReceived
bool
DropAnticipationState::MessageReceived(BMessage* message, Command** _command)
{
	if (message->what == fDragMessageClone.what) {
		*_command = HandleDropMessage(message);
		return *_command != NULL;
	}
	return false;
}

// #pragma mark -

// MouseMoved
void
DropAnticipationState::MouseMoved(BPoint where, uint32 transit,
								  const BMessage* dragMessage)
{
	if (!dragMessage) {
		fprintf(stderr, "DragAnticipationState::MouseMoved() "
						"with no buttons pressed or no drag message!\n");
		return;
	}
	// clone the message if appropriate
	// NOTE: we might need to look at the dragMessage
	// again in ModifiersChanged(), that's why we need
	// to clone it
	if (dragMessage->what != fDragMessageClone.what) {
		if (WouldAcceptDragMessage(dragMessage))
			_SetMessage(dragMessage);
	}
	if (fDragMessageClone.what != 0) {
		// we already figured out that we would accept the message
		if (fView->Bounds().Contains(where))
			UpdateDropIndication(dragMessage, where, Modifiers());
		else
			RemoveDropAnticipationRect();
	}
}

// MouseUp
Command*
DropAnticipationState::MouseUp()
{
	Command* command = NULL;
	// it is a bit hacky here to not wait for the drop message
	// to show up in MessageReceived(), but have not yet come
	// up with a better idea. The state is "temporary" and will
	// be removed after MouseUp is returns
	if (fDragMessageClone.what != 0
		&& fView->Bounds().Contains(MousePos())) {
		command = HandleDropMessage(&fDragMessageClone);
	}

	_UnsetMessage();
	RemoveDropAnticipationRect();

	return command;
}

// ModifiersChanged
void
DropAnticipationState::ModifiersChanged(uint32 modifiers)
{
	// NOTE: this gives subclasses a chance to
	// indicate different drop action
	// depending on held modifiers
	UpdateDropIndication(&fDragMessageClone, MousePos(), modifiers);
}

// #pragma mark -

// WouldAcceptDragMessage
bool
DropAnticipationState::WouldAcceptDragMessage(const BMessage* dragMessage)
{
	// yeah, I take anything from you
	return true;
}

// HandleDropMessage
Command*
DropAnticipationState::HandleDropMessage(BMessage* dropMessage)
{
	// we're too dumb to handle it ourselves
	fView->Window()->PostMessage(dropMessage);
	return NULL;
}

// UpdateDropIndication
void
DropAnticipationState::UpdateDropIndication(const BMessage* dragMessage,
											BPoint where, uint32 modifiers)
{
	// simplistic implementation
	BRect bounds = fView->Bounds();
	if (bounds.Contains(where)) {
		SetDropAnticipationRect(bounds);
	} else {
		RemoveDropAnticipationRect();
	}
}

// SetDropAnticipationRect
void
DropAnticipationState::SetDropAnticipationRect(BRect rect)
{
	if (rect != fDropAnticipationRect) {
		if (fDropAnticipationRect.IsValid())
			fView->Invalidate(fDropAnticipationRect);

		fDropAnticipationRect = rect;

		if (fDropAnticipationRect.IsValid())
			fView->Invalidate(fDropAnticipationRect);
	}
}

// RemoveDropAnticipationRect
void
DropAnticipationState::RemoveDropAnticipationRect()
{
	SetDropAnticipationRect(BRect(0, 0, -1, -1));
}

// #pragma mark -

// _SetMessage
void
DropAnticipationState::_SetMessage(const BMessage* message)
{
	fDragMessageClone = *message;
}

// _UnsetMessage
void
DropAnticipationState::_UnsetMessage()
{
	fDragMessageClone.MakeEmpty();
	fDragMessageClone.what = 0;
}



/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "InputTextView.h"

#include <stdio.h>
#include <stdlib.h>

#include <String.h>
#include <Window.h>

#include "ui_defines.h"

// constructor
InputTextView::InputTextView(BRect frame, const char* name,
							 BRect textRect,
							 uint32 resizingMode,
							 uint32 flags)
	: BTextView(frame, name, textRect, resizingMode, flags),
	  fWasFocus(false)
{
	SetWordWrap(false);
}

// destructor
InputTextView::~InputTextView()
{
}

// MouseDown
void
InputTextView::MouseDown(BPoint where)
{
	// enforce the behaviour of a typical BTextControl
	// only let the BTextView handle mouse up/down when
	// it already had focus
	fWasFocus = IsFocus();
	if (fWasFocus) {
		BTextView::MouseDown(where);
	} else {
		// forward click
		if (BView* view = Parent()) {
			view->MouseDown(ConvertToParent(where));
		}
	}
}

// MouseUp
void
InputTextView::MouseUp(BPoint where)
{
	// enforce the behaviour of a typical BTextControl
	// only let the BTextView handle mouse up/down when
	// it already had focus
	if (fWasFocus)
		BTextView::MouseUp(where);
}

// KeyDown
void
InputTextView::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = true;
	if (numBytes > 0) {
		switch (bytes[0]) {
			case B_ESCAPE:
				// revert any typing changes
				RevertChanges();
				break;
			case B_TAB:
				// skip BTextView implementation
				BView::KeyDown(bytes, numBytes);
				// fall through
			case B_RETURN:
				ApplyChanges();
				break;
			default:
				handled = false;
				break;
		}
	}
	if (!handled)
		BTextView::KeyDown(bytes, numBytes);
}

// MakeFocus
void
InputTextView::MakeFocus(bool focus)
{
	if (focus != IsFocus()) {
		if (BView* view = Parent())
			view->Invalidate();
		BTextView::MakeFocus(focus);
		if (Window()) {
			BMessage focusMessage(MSG_FOCUS_CHANGED);
			focusMessage.AddPointer("source", (BView*)this);
			focusMessage.AddBool("focus", focus);
			Window()->PostMessage(&focusMessage);
				// send the focus change message to the
				// window, not our normal target
		}
		if (focus)
			SelectAll();
		else
			ApplyChanges();
	}
}

// Invoke
status_t
InputTextView::Invoke(BMessage* message)
{
	if (!message)
		message = Message();

	if (message) {
		BMessage copy(*message);
		copy.AddInt64("when", system_time());
		copy.AddPointer("source", (BView*)this);
		return BInvoker::Invoke(&copy);
	}
	return B_BAD_VALUE;
}

// #pragma mark -

// Select
void
InputTextView::Select(int32 start, int32 finish)
{
	BTextView::Select(start, finish);

	_CheckTextRect();
}

// InsertText
void
InputTextView::InsertText(const char* inText, int32 inLength, int32 inOffset,
						  const text_run_array* inRuns)
{
	BTextView::InsertText(inText, inLength, inOffset, inRuns);

	_CheckTextRect();
}

// DeleteText
void
InputTextView::DeleteText(int32 fromOffset, int32 toOffset)
{
	BTextView::DeleteText(fromOffset, toOffset);

	_CheckTextRect();
}

// #pragma mark -

// _CheckTextRect
void
InputTextView::_CheckTextRect()
{
	// update text rect and make sure
	// the cursor/selection is in view
	BRect textRect(TextRect());
	float width = ceilf(StringWidth(Text()) + 2.0);
	if (textRect.Width() != width) {
		textRect.right = textRect.left + width;
		SetTextRect(textRect);
		ScrollToSelection();

		if (Window())
			Invalidate();
	}
}

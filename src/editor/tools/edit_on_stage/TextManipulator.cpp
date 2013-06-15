/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TextManipulator.h"

#include <new>
#include <stdio.h>

#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <View.h>

#include "ui_defines.h"
#include "support_utf8.h"

#include "CommonPropertyIDs.h"
#include "Observable.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyCommand.h"
#include "Selection.h"

using std::nothrow;

enum {
	DRAGGING_NONE = 0,
	DRAGGING_PARAGRAPH_INSET,
	DRAGGING_BLOCK_WIDTH,
};

enum {
	MSG_BLINK_CURSOR = 'blnk',
};

// constructor
TextManipulator::TextManipulator(PlaylistItem* item, TextClip* text,
		Selection* selection)
	: Manipulator(item)

	, fSelection(selection)
	, fPlaylistItem(item)
	, fTextClip(text)
	, fTransform(item->Transformation())

	, fDragMode(DRAGGING_NONE)

	, fCursorPosition(0)
	, fShowCursor(true)

	, fOldBounds(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN)

	, fCursorBlinkPulse(NULL)
{
	fTextClip->AddObserver(this);
}

// destructor
TextManipulator::~TextManipulator()
{
	fTextClip->RemoveObserver(this);
	delete fCursorBlinkPulse;
}

// #pragma mark -

// Draw
void
TextManipulator::Draw(BView* into, BRect updateRect)
{
	into->SetHighColor(kBlack);
	into->SetLowColor(kWhite);

	BPoint start(0.0, 0.0);
	BPoint paragraphInset(fTextClip->ParagraphInset(), 0.0);
	BPoint blockWidth(fTextClip->BlockWidth(), 0.0);

	_StrokeLine(into, start, blockWidth);
	_StrokeDot(into, paragraphInset);
	_StrokeRect(into, blockWidth);

	if (!fView->IsFocus() || !fShowCursor)
		return;

	BRect cursorBounds = _CursorBounds();
	_StrokeLine(into, cursorBounds.LeftTop(), cursorBounds.LeftBottom());
}

// MouseDown
bool
TextManipulator::MouseDown(BPoint where)
{
	if (!fTransform.IsValid() || fDragMode != DRAGGING_NONE)
		return false;

	fView->ConvertToCanvas(&where);
	fTransform.InverseTransform(&where);

	float hitTestInset = 7.0 / fView->ZoomLevel();

	BPoint paragraphInset(fTextClip->ParagraphInset(), 0.0);
	BRect hitTest(paragraphInset, paragraphInset);
	hitTest.InsetBy(-hitTestInset, -hitTestInset);
	if (hitTest.Contains(where)) {
		fDragMode = DRAGGING_PARAGRAPH_INSET;
		fDragStart = where;
		fDragStartValue = fTextClip->ParagraphInset();
		return true;
	}


	BPoint blockWidth(fTextClip->BlockWidth(), 0.0);
	hitTest.Set(blockWidth.x, blockWidth.y, blockWidth.x, blockWidth.y);
	hitTest.InsetBy(-hitTestInset, -hitTestInset);
	if (hitTest.Contains(where)) {
		fDragMode = DRAGGING_BLOCK_WIDTH;
		fDragStart = where;
		fDragStartValue = fTextClip->BlockWidth();
		return true;
	}

	return false;
}

// MouseMoved
void
TextManipulator::MouseMoved(BPoint where)
{
	if (!fTransform.IsValid())
		return;

	fView->ConvertToCanvas(&where);
	fTransform.InverseTransform(&where);

	switch (fDragMode) {
		default:
		case DRAGGING_NONE:
			return;
		case DRAGGING_PARAGRAPH_INSET: {
			float offset = where.x - fDragStart.x;
			float newInset = max_c(0.0, max_c(0, fDragStartValue + offset));
			FloatProperty* property = dynamic_cast<FloatProperty*>(
				fTextClip->FindProperty(PROPERTY_PARAGRAPH_INSET));
			_ChangeProperty(property, newInset);
			break;
		}
		case DRAGGING_BLOCK_WIDTH: {
			float offset = where.x - fDragStart.x;
			float newWidth = max_c(5.0, fDragStartValue + offset);
			FloatProperty* property = dynamic_cast<FloatProperty*>(
				fTextClip->FindProperty(PROPERTY_BLOCK_WIDTH));
			_ChangeProperty(property, newWidth);
			break;
		}
	}
}

// MouseUp
Command*
TextManipulator::MouseUp()
{
	fDragMode = DRAGGING_NONE;
	return NULL;
}

// MouseOver
bool
TextManipulator::MouseOver(BPoint where)
{
	return false;
}

// DoubleClicked
bool
TextManipulator::DoubleClicked(BPoint where)
{
	return false;
}

// #pragma mark -

// ModifiersChanged
void
TextManipulator::ModifiersChanged(uint32 modifiers)
{
}

// MessageReceived
bool
TextManipulator::MessageReceived(BMessage* message, Command** _command)
{
	switch (message->what) {
		case MSG_BLINK_CURSOR:
			if (fView->IsFocus()) {
				_InvalidateCursorBounds();
				fShowCursor = !fShowCursor;
			}
			return true;
		default:
			return Manipulator::MessageReceived(message, _command);
	}
}

// HandleKeyDown
bool
TextManipulator::HandleKeyDown(const StateView::KeyEvent& event,
								Command** _command)
{
	BString text = fTextClip->Text();
	int32 cursorPosition = fCursorPosition;
	bool textChanged = false;

	switch (event.key) {
		case B_HOME:
			cursorPosition = fTextClip->FirstGlyphIndexAtLine(cursorPosition);
			break;
		case B_END:
			cursorPosition = fTextClip->LastGlyphIndexAtLine(cursorPosition);
			break;

		case B_PAGE_UP:
		case B_PAGE_DOWN:
			return false;

		case B_UP_ARROW:
			cursorPosition = fTextClip->NextGlyphAtLineOffset(
				cursorPosition, -1);
			break;
		case B_DOWN_ARROW:
			cursorPosition = fTextClip->NextGlyphAtLineOffset(
				cursorPosition, 1);
			break;
		case B_LEFT_ARROW:
			cursorPosition--;
			break;
		case B_RIGHT_ARROW:
			cursorPosition++;
			break;

		case B_DELETE:
			text = _Cut(text, cursorPosition, cursorPosition + 1);
			textChanged = true;
			break;

		case B_BACKSPACE:
			text = _Cut(text, cursorPosition - 1, cursorPosition);
			cursorPosition--;
			textChanged = true;
			break;

		default:
			// TODO: check if this is a glyph at all (printable char)
			text = _Insert(text, event.bytes, cursorPosition);
			cursorPosition++;
			textChanged = true;
			break;
	}

	if (textChanged) {
		StringProperty* property = dynamic_cast<StringProperty*>(
			fTextClip->FindProperty(PROPERTY_TEXT));
		_ChangeProperty(property, text.String());
	}

	fView->TriggerUpdate();
	_SetCursorPos(cursorPosition);

	return true;
}

// HandleKeyUp
bool
TextManipulator::HandleKeyUp(const StateView::KeyEvent& event,
							  Command** _command)
{
	return false;
}

// HandlesAllKeyEvents
bool
TextManipulator::HandlesAllKeyEvents() const
{
	return true;
}

// UpdateCursor
bool
TextManipulator::UpdateCursor()
{
	// TODO...
	return false;
}

// #pragma mark -

// Bounds
BRect
TextManipulator::Bounds()
{
	BRect dummy;
	BRect bounds = fTextClip->Bounds(dummy);
	bounds.left = min_c(0, bounds.left);
	bounds.top = min_c(0, bounds.top);
	bounds.right = max_c(bounds.right, fTextClip->BlockWidth());
	bounds.InsetBy(-5, -5);
	bounds = fTransform.TransformBounds(bounds);
	fView->ConvertFromCanvas(&bounds);
	return bounds;
}

// AttachedToView
void
TextManipulator::AttachedToView(StateView* view)
{
	BMessenger messenger(view);

	BMessage message(MSG_BLINK_CURSOR);
	delete fCursorBlinkPulse;
	fCursorBlinkPulse = new BMessageRunner(messenger, &message, 500000);
}

// DetachedFromView
void
TextManipulator::DetachedFromView(StateView* view)
{
	delete fCursorBlinkPulse;
	fCursorBlinkPulse = NULL;
}

// #pragma mark -

void
TextManipulator::ObjectChanged(const Observable* object)
{
	if (object == fPlaylistItem) {
		AffineTransform transform = fPlaylistItem->Transformation();
		if (transform != fTransform) {
			fTransform = transform;

			BRect newBounds = Bounds();
			Invalidate(fOldBounds | newBounds);
			fOldBounds = newBounds;
		}
	} else if (object == fTextClip) {
		BRect newBounds = Bounds();
		Invalidate(fOldBounds | newBounds);
		fOldBounds = newBounds;
	}
}

//// TableDataChanged
//void
//TextManipulator::TableDataChanged(const TableData::Event& event)
//{
//	bool invalidate = true;
//
//	switch (event.property) {
//		case TABLE_CELL_TEXT:
//printf("TextManipulator::TableDataChanged(TABLE_CELL_TEXT)\n");
//			break;
//		case TABLE_CELL_BACKGROUND_COLOR:
//		case TABLE_CELL_CONTENT_COLOR:
//		case TABLE_CELL_HORIZONTAL_ALIGNMENT:
//		case TABLE_CELL_VERTICAL_ALIGNMENT:
//			invalidate = false;
//			break;
//		case TABLE_CELL_FONT:
//printf("TextManipulator::TableDataChanged(TABLE_CELL_FONT)\n");
//			break;
//		case TABLE_COLUMN_WIDTH:
//printf("TextManipulator::TableDataChanged(TABLE_COLUMN_WIDTH)\n");
//			break;
//		case TABLE_ROW_HEIGHT:
//printf("TextManipulator::TableDataChanged(TABLE_ROW_HEIGHT)\n");
//			break;
//		case TABLE_DIMENSIONS:
//printf("TextManipulator::TableDataChanged(TABLE_DIMENSIONS)\n");
//			break;
//	}
//
//	if (invalidate) {
//		BRect newBounds = Bounds();
//		Invalidate(fOldBounds | newBounds);
//		fOldBounds = newBounds;
//	}
//}

// _StrokeLine
void
TextManipulator::_StrokeLine(BView* into, BPoint a, BPoint b)
{
	// transform on canvas
	fTransform.Transform(&a);
	fTransform.Transform(&b);
	// transform to view
	fView->ConvertFromCanvas(&a);
	fView->ConvertFromCanvas(&b);
	into->StrokeLine(a, b, kDotted);
}

// _StrokeDot
void
TextManipulator::_StrokeDot(BView* into, BPoint a)
{
	// transform on canvas
	fTransform.Transform(&a);
	// transform to view
	fView->ConvertFromCanvas(&a);

	BRect dot(a, a);
	dot.InsetBy(-3, -3);

	into->FillEllipse(dot, B_SOLID_LOW);

	dot.InsetBy(-1, -1);
	into->StrokeEllipse(dot, B_SOLID_HIGH);
}

// _StrokeRect
void
TextManipulator::_StrokeRect(BView* into, BPoint a)
{
	// transform on canvas
	fTransform.Transform(&a);
	// transform to view
	fView->ConvertFromCanvas(&a);

	BRect rect(a, a);
	rect.InsetBy(-3, -3);

	into->FillRect(rect, B_SOLID_LOW);

	rect.InsetBy(-1, -1);
	into->StrokeRect(rect, B_SOLID_HIGH);
}

// _SetCursorPos
void
TextManipulator::_SetCursorPos(int32 newPos)
{
	if (newPos < 0)
		newPos = 0;

	BString text = fTextClip->Text();
	int32 maxPos = UTF8CountChars(text.String(), -1);
	if (newPos > maxPos)
		newPos = maxPos;

	if (newPos == fCursorPosition)
		return;

	fShowCursor = true;

	_InvalidateCursorBounds();

	fCursorPosition = newPos;

	_InvalidateCursorBounds();
}

// _CursorBounds
BRect
TextManipulator::_CursorBounds() const
{
	BPoint cursorBottom;

	fTextClip->GetBaselinePositions(fCursorPosition,
		fCursorPosition, &cursorBottom, NULL);

	BPoint cursorTop = cursorBottom + BPoint(0.0, -fTextClip->FontSize());

	return BRect(cursorTop, cursorBottom);
}

// _InvalidateCursorBounds
void
TextManipulator::_InvalidateCursorBounds()
{
	BRect cursorBounds = _CursorBounds();
	cursorBounds.InsetBy(-1, -1);
	if (fTransform.IsValid())
		cursorBounds = fTransform.TransformBounds(cursorBounds);
	fView->ConvertFromCanvas(&cursorBounds);
	Invalidate(cursorBounds);
}

// #pragma mark -

// _Cut
BString
TextManipulator::_Cut(const BString& text, int32 start, int32 end) const
{
	BString newText;
	const char* t = text.String();
	uint32 bytesUntilStart = UTF8CountBytes(t, start);
	uint32 bytesRange = UTF8CountBytes(t + bytesUntilStart, end - start);
	uint32 bytesUntilEnd = UTF8CountBytes(t + bytesUntilStart
		 + bytesRange, -1);

	if (bytesUntilStart > 0) {
		char buffer[bytesUntilStart + 1];
		memcpy(buffer, t, bytesUntilStart);
		buffer[bytesUntilStart] = 0;
		newText << buffer;
	}

	if (bytesUntilEnd > 0) {
		char buffer[bytesUntilEnd + 1];
		memcpy(buffer, t + bytesUntilStart
			 + bytesRange, bytesUntilEnd);
		buffer[bytesUntilEnd] = 0;
		newText << buffer;
	}

	return newText;
}

// _Insert
BString
TextManipulator::_Insert(const BString& text, const BString& insert,
	int32 position) const
{
	BString newText;
	const char* t = text.String();
	uint32 bytesUntilStart = UTF8CountBytes(t, position);
	uint32 bytesUntilEnd = UTF8CountBytes(t + bytesUntilStart, -1);

	if (bytesUntilStart > 0) {
		char buffer[bytesUntilStart + 1];
		memcpy(buffer, t, bytesUntilStart);
		buffer[bytesUntilStart] = 0;
		newText << buffer;
	}

	newText << insert;

	if (bytesUntilEnd > 0) {
		char buffer[bytesUntilEnd + 1];
		memcpy(buffer, t + bytesUntilStart, bytesUntilEnd);
		buffer[bytesUntilEnd] = 0;
		newText << buffer;
	}

	return newText;
}

// #pragma mark -

// _ChangeProperty
template<class PropertyType, class ValueType>
void
TextManipulator::_ChangeProperty(PropertyType* property,
	ValueType value)
{
	if (!property)
		return;

	Property* clonedProperty = property->Clone(false);
	if (!clonedProperty)
		return;

	PropertyCommand* command
		= new (nothrow) PropertyCommand(clonedProperty, fTextClip);

	if (property->SetValue(value))
		fTextClip->ValueChanged(property);

	fView->PerformCommand(command);
}


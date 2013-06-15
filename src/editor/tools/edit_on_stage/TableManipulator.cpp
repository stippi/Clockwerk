/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TableManipulator.h"

#include <stdio.h>

#include <View.h>

#include "ui_defines.h"

#include "Observable.h"
#include "PlaylistItem.h"
#include "Selection.h"

enum {
	DRAGGING_NONE = 0,
	DRAGGING_COLUMN,
	DRAGGING_ROW,
};

// constructor
TableManipulator::TableManipulator(PlaylistItem* item,
								   TableClip* table,
								   Selection* selection)
	: Manipulator(item),

	  fSelection(selection),
	  fPlaylistItem(item),
	  fTableClip(table),
	  fTransform(item->Transformation()),

	  fDragMode(DRAGGING_NONE),
	  fDragIndex(-1),

	  fFocusedColumn(-1),
	  fFocusedRow(-1),

	  fOldBounds(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN)
{
	fTableClip->Table().AddListener(this);
	
}

// destructor
TableManipulator::~TableManipulator()
{
	fTableClip->Table().RemoveListener(this);
}

// #pragma mark -

// Draw
void
TableManipulator::Draw(BView* into, BRect updateRect)
{
	BRect bounds = fTableClip->Table().Bounds();

	into->SetHighColor(kBlack);
	into->SetLowColor(kWhite);

	uint32 columns = fTableClip->Table().CountColumns();
	float pos = 0.0;
	for (uint32 i = 0; i <= columns; i++) {
		_StrokeLine(into, BPoint(pos, bounds.top),
						  BPoint(pos, bounds.bottom));
		if (i < columns)
			pos += fTableClip->Table().ColumnWidth(i);
	}

	uint32 rows = fTableClip->Table().CountRows();
	pos = 0.0;
	for (uint32 i = 0; i <= rows; i++) {
		_StrokeLine(into, BPoint(bounds.left, pos),
						  BPoint(bounds.right, pos));
		if (i < rows)
			pos += fTableClip->Table().RowHeight(i);
	}

	if (fFocusedColumn >= 0 && fFocusedRow >= 0) {
		BRect cellBounds
			= fTableClip->Table().Bounds(fFocusedColumn, fFocusedRow);
		cellBounds.left++;
		cellBounds.top++;
		into->SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		_StrokeRect(into, cellBounds);
		cellBounds.InsetBy(1, 1);
		_StrokeRect(into, cellBounds);
	}
}

// MouseDown
bool
TableManipulator::MouseDown(BPoint where)
{
	if (!fTransform.IsValid() || fDragMode != DRAGGING_NONE)
		return false;

	fTransform.InverseTransform(&where);
	
	BRect bounds = fTableClip->Table().Bounds();
	float hitDist = 4.0 / fTransform.scale();
	bounds.InsetBy(-hitDist, -hitDist);
	if (!bounds.Contains(where))
		return false;

	// find dragged column/row
	uint32 columns = fTableClip->Table().CountColumns();
	float pos = 0.0;
	for (uint32 i = 0; i <= columns; i++) {
		if (i > 0 && fabs(where.x - pos) < hitDist) {
			fDragMode = DRAGGING_COLUMN;
			fDragIndex = i - 1;
			fDragStart = where;
			fDragStartSize = fTableClip->Table().ColumnWidth(fDragIndex);
			return true;
		}
		if (i < columns)
			pos += fTableClip->Table().ColumnWidth(i);
	}

	uint32 rows = fTableClip->Table().CountRows();
	pos = 0.0;
	for (uint32 i = 0; i <= rows; i++) {
		if (i > 0 && fabs(where.y - pos) < hitDist) {
			fDragMode = DRAGGING_ROW;
			fDragIndex = i - 1;
			fDragStart = where;
			fDragStartSize = fTableClip->Table().RowHeight(fDragIndex);
			return true;
		}
		if (i < rows)
			pos += fTableClip->Table().RowHeight(i);
	}


	// nothing being dragged - find clicked column/row
	int32 newFocusedColumn = -1;
	int32 newFocusedRow = -1;

	pos = 0.0;
	for (uint32 i = 0; i < columns; i++) {
		if (pos > where.x)
			break;

		pos += fTableClip->Table().ColumnWidth(i);
		newFocusedColumn++;
	}
	if (pos < where.x) {
		newFocusedColumn = -1;
	}

	pos = 0.0;
	for (uint32 i = 0; i < rows; i++) {
		if (pos > where.y)
			break;

		pos += fTableClip->Table().RowHeight(i);
		newFocusedRow++;
	}
	if (pos < where.y) {
		newFocusedRow = -1;
		newFocusedColumn = -1;
	}

	_SetFocusCell(newFocusedColumn, newFocusedRow);

	return false;
}

// MouseMoved
void
TableManipulator::MouseMoved(BPoint where)
{
	if (!fTransform.IsValid())
		return;

	fTransform.InverseTransform(&where);

	switch (fDragMode) {
		default:
		case DRAGGING_NONE:
			return;
		case DRAGGING_COLUMN: {
			float offset = where.x - fDragStart.x;
			float newWidth = max_c(5.0, fDragStartSize + offset);
			fTableClip->Table().SetColumnWidth(fDragIndex, newWidth);
			break;
		}
		case DRAGGING_ROW: {
			float offset = where.y - fDragStart.y;
			float newHeight = max_c(5.0, fDragStartSize + offset);
			fTableClip->Table().SetRowHeight(fDragIndex, newHeight);
			break;
		}
	}
}

// MouseUp
Command*
TableManipulator::MouseUp()
{
	fDragMode = DRAGGING_NONE;
	return NULL;
}

// MouseOver
bool
TableManipulator::MouseOver(BPoint where)
{
	return false;
}

// DoubleClicked
bool
TableManipulator::DoubleClicked(BPoint where)
{
	return false;
}

// #pragma mark -

// ModifiersChanged
void
TableManipulator::ModifiersChanged(uint32 modifiers)
{
}

// HandleKeyDown
bool
TableManipulator::HandleKeyDown(const StateView::KeyEvent& event,
								Command** _command)
{
	// navigation between cells
	switch (event.key) {
		case B_RETURN:
			_FocusNextCell();
			return true;
		case B_TAB:
			if (event.modifiers & B_SHIFT_KEY)
				_FocusPreviousCell();
			else
				_FocusNextCell();
			return true;
	}

	// navigation or typing within a cell
	if (fFocusedColumn < 0 || fFocusedRow < 0)
		return false;

	BString text = fTableClip->Table().CellText(fFocusedColumn, fFocusedRow);

	switch (event.key) {
		case B_UP_ARROW:
		case B_DOWN_ARROW:
		case B_LEFT_ARROW:
		case B_RIGHT_ARROW:
			// TODO: move carret (or navigate cell)
			return true;

		case B_BACKSPACE:
			text.Truncate(max_c(0, text.CountChars() - 1));
			// NOTE: BeBook says "charCount" but is
			// probably "bytes"... so it would needto be
			// fixed for UTF8
			break;

		default:
			text << event.bytes;
			break;
	}

	fTableClip->Table().SetCellText(fFocusedColumn, fFocusedRow, text);
	TriggerUpdate();

	return false;
}

// HandleKeyUp
bool
TableManipulator::HandleKeyUp(const StateView::KeyEvent& event,
							  Command** _command)
{
	return false;
}

// UpdateCursor
bool
TableManipulator::UpdateCursor()
{
	// TODO...
	return false;
}

// #pragma mark -

// Bounds
BRect
TableManipulator::Bounds()
{
	BRect bounds = fTableClip->Table().Bounds();
	return fTransform.TransformBounds(bounds.InsetByCopy(-1, -1));
}

// #pragma mark -

void
TableManipulator::ObjectChanged(const Observable* object)
{
	AffineTransform transform = fPlaylistItem->Transformation();
	if (transform != fTransform) {
		fTransform = transform;

		BRect newBounds = Bounds();
		Invalidate(fOldBounds | newBounds);
		fOldBounds = newBounds;
	}
}

// TableDataChanged
void
TableManipulator::TableDataChanged(const TableData::Event& event)
{
	bool invalidate = true;

	switch (event.property) {
		case TABLE_CELL_TEXT:
printf("TableManipulator::TableDataChanged(TABLE_CELL_TEXT)\n");
			break;
		case TABLE_CELL_BACKGROUND_COLOR:
		case TABLE_CELL_CONTENT_COLOR:
		case TABLE_CELL_HORIZONTAL_ALIGNMENT:
		case TABLE_CELL_VERTICAL_ALIGNMENT:
			invalidate = false;
			break;
		case TABLE_CELL_FONT:
printf("TableManipulator::TableDataChanged(TABLE_CELL_FONT)\n");
			break;
		case TABLE_COLUMN_WIDTH:
printf("TableManipulator::TableDataChanged(TABLE_COLUMN_WIDTH)\n");
			break;
		case TABLE_ROW_HEIGHT:
printf("TableManipulator::TableDataChanged(TABLE_ROW_HEIGHT)\n");
			break;
		case TABLE_DIMENSIONS:
printf("TableManipulator::TableDataChanged(TABLE_DIMENSIONS)\n");
			break;
	}

	if (invalidate) {
		BRect newBounds = Bounds();
		Invalidate(fOldBounds | newBounds);
		fOldBounds = newBounds;
	}
}

// _StrokeLine
void
TableManipulator::_StrokeLine(BView* into, BPoint a, BPoint b)
{
	// transform on canvas
	fTransform.Transform(&a);
	fTransform.Transform(&b);
	// transform to view
	fView->ConvertFromCanvas(&a);
	fView->ConvertFromCanvas(&b);
	into->StrokeLine(a, b, kDotted);
}

// _StrokeRect
void
TableManipulator::_StrokeRect(BView* into, BRect rect)
{
	_StrokeLine(into, rect.LeftTop(), rect.RightTop());
	_StrokeLine(into, rect.RightTop(), rect.RightBottom());
	_StrokeLine(into, rect.RightBottom(), rect.LeftBottom());
	_StrokeLine(into, rect.LeftBottom(), rect.LeftTop());
}

// _FocusNextCell
void
TableManipulator::_FocusNextCell()
{
	if (fFocusedColumn < 0 || fFocusedRow < 0) {
		_SetFocusCell(0, 0);
		return;
	}

	int32 columns = fTableClip->Table().CountColumns();
	int32 rows = fTableClip->Table().CountRows();

	int32 newColumn = fFocusedColumn + 1;
	int32 newRow = fFocusedRow;

	if (newColumn >= columns) {
		newColumn = 0;
		newRow++;
		if (newRow >= rows)
			newRow = 0;
	}

	_SetFocusCell(newColumn, newRow);
}

// _FocusPreviousCell
void
TableManipulator::_FocusPreviousCell()
{
	int32 columns = fTableClip->Table().CountColumns();
	int32 rows = fTableClip->Table().CountRows();

	if (fFocusedColumn < 0 || fFocusedRow < 0) {
		_SetFocusCell(columns - 1, rows - 1);
		return;
	}

	int32 newColumn = fFocusedColumn - 1;
	int32 newRow = fFocusedRow;

	if (newColumn < 0) {
		newColumn = columns - 1;
		newRow--;
		if (newRow < 0)
			newRow = rows - 1;
	}

	_SetFocusCell(newColumn, newRow);
}

// _SetFocusCell
void
TableManipulator::_SetFocusCell(int32 column, int32 row)
{
	if (column < 0 || row < 0) {
		column = -1;
		row = -1;
	}

	if (fFocusedColumn == column && fFocusedRow == row)
		return;

	fFocusedColumn = column;
	fFocusedRow = row;

	if (fSelection) {
		fSelection->DeselectAll();
		if (fFocusedColumn >= 0) {
			fSelection->Select(fTableClip->Table().SelectCell(fFocusedColumn,
															  fFocusedRow));
		}
	} else {
		printf("no Selection!\n");
	}

	Invalidate();
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TABLE_MANIPULATOR_H
#define TABLE_MANIPULATOR_H

#include "AffineTransform.h"
#include "Manipulator.h"
#include "TableClip.h"

class PlaylistItem;
class Selection;

class TableManipulator : public Manipulator,
						 public TableData::Listener {
 public:
								TableManipulator(PlaylistItem* item,
												 TableClip* table,
												 Selection* selection);
	virtual						~TableManipulator();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

	virtual	void				ModifiersChanged(uint32 modifiers);
	virtual	bool				HandleKeyDown(
									const StateView::KeyEvent& event,
									Command** _command);
	virtual	bool				HandleKeyUp(
									const StateView::KeyEvent& event,
									Command** _command);

	virtual	bool				UpdateCursor();

	virtual	BRect				Bounds();
		// the area that the manipulator is
		// occupying in the "parent" view

	// Observer interface (Manipulator)
	virtual	void				ObjectChanged(const Observable* object);

	// TableData::Listener interface
	virtual	void				TableDataChanged(
									const TableData::Event& event);

 private:
			void				_StrokeLine(BView* into, BPoint a, BPoint b);
			void				_StrokeRect(BView* into, BRect rect);

			void				_FocusNextCell();
			void				_FocusPreviousCell();
			void				_SetFocusCell(int32 column, int32 row);

			class SelectableCell;

			Selection*			fSelection;
			PlaylistItem*		fPlaylistItem;
			TableClip*			fTableClip;
			AffineTransform		fTransform;
			
			uint32				fDragMode;
			int32				fDragIndex;
			BPoint				fDragStart;
			float				fDragStartSize;

			int32				fFocusedColumn;
			int32				fFocusedRow;

			BRect				fOldBounds;
};

#endif // TABLE_MANIPULATOR_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TEXT_MANIPULATOR_H
#define TEXT_MANIPULATOR_H


#include "AffineTransform.h"
#include "Manipulator.h"
#include "TextClip.h"


class BMessageRunner;
class PlaylistItem;
class Selection;


class TextManipulator : public Manipulator {
 public:
								TextManipulator(PlaylistItem* item,
									TextClip* text, Selection* selection);
	virtual						~TextManipulator();

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

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

	virtual	BRect				Bounds();
		// the area that the manipulator is
		// occupying in the "parent" view

	virtual	void				AttachedToView(StateView* view);
	virtual	void				DetachedFromView(StateView* view);

	// Observer interface (Manipulator)
	virtual	void				ObjectChanged(const Observable* object);


 private:
			void				_StrokeLine(BView* into, BPoint a, BPoint b);
			void				_StrokeDot(BView* into, BPoint a);
			void				_StrokeRect(BView* into, BPoint a);

			void				_SetCursorPos(int32 position);
			BRect				_CursorBounds() const;
			void				_InvalidateCursorBounds();

			BString				_Cut(const BString& text, int32 start,
									int32 end) const;
			BString				_Insert(const BString& text,
									const BString& insert,
									int32 position) const;

			template<class PropertyType, class ValueType>
			void				_ChangeProperty(PropertyType* property,
									ValueType value);

			Selection*			fSelection;
			PlaylistItem*		fPlaylistItem;
			TextClip*			fTextClip;
			AffineTransform		fTransform;
			
			uint32				fDragMode;
			BPoint				fDragStart;
			float				fDragStartValue;

			int32				fCursorPosition;
			bool				fShowCursor;

			BRect				fOldBounds;

			BMessageRunner*		fCursorBlinkPulse;
};

#endif // TEXT_MANIPULATOR_H

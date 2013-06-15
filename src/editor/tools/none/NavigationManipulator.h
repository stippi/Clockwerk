/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NAVIGATION_MANIPULATOR_H
#define NAVIGATION_MANIPULATOR_H


#include "Manipulator.h"


class Playlist;


class NavigationManipulator : public Manipulator {
 public:
								NavigationManipulator(Playlist* playlist);
	virtual						~NavigationManipulator();

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

	virtual	void				ObjectChanged(const Observable*);

	// StageManipulator interface
	virtual	void				SetCurrentFrame(int64 frame);

 private:
			Playlist*			fPlaylist;
			int64				fCurrentFrame;
};

#endif // NAVIGATION_MANIPULATOR_H

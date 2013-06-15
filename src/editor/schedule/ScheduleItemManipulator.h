/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_ITEM_MANIPULATOR_H
#define SCHEDULE_ITEM_MANIPULATOR_H

#include <String.h>

#include "Manipulator.h"

class ScheduleItem;
class ScheduleView;

class ScheduleItemManipulator : public Manipulator {
 public:
								ScheduleItemManipulator(ScheduleItem* item);
	virtual						~ScheduleItemManipulator();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

	virtual	bool				MessageReceived(BMessage* message,
									Command** _command);

	virtual	bool				UpdateCursor();

	virtual	BRect				Bounds();

	virtual	void				AttachedToView(StateView* view);
	virtual	void				DetachedFromView(StateView* view);

	virtual	void				RebuildCachedData();

	// ScheduleItemManipulator
			ScheduleItem*		Item() const
									{ return fItem; }

			void				SetHighlighted(bool highlighted,
									bool invalidate = true);

 protected:
			void				_ObjectChanged();
			BRect				_ComputeFrameFor(ScheduleItem* item) const;

			ScheduleItem*		fItem;
			BRect				fItemFrame;

			uint64				fCachedStartFrame;
			uint64				fCachedDuration;
			BString				fCachedName;

			bool				fCachedSelected : 1;
			bool				fCachedFlexibleDuration : 1;

			bool				fPreparedToResize : 1;
			bool				fHighlighted : 1;

			ScheduleView*		fView;

			uint32				fDragMode;
};

#endif // SCHEDULE_ITEM_MANIPULATOR_H

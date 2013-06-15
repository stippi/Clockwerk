/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PROPERTY_MANIPULATOR_H
#define PROPERTY_MANIPULATOR_H

#include "List.h"
#include "Manipulator.h"

class BPoint;
class KeyFrame;
class SplitManipulator;
class PropertyAnimator;

class PropertyManipulator : public Manipulator {
 public:
								PropertyManipulator(
									SplitManipulator* parent,
									PropertyAnimator* animator);
	virtual						~PropertyManipulator();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// Manipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);

	virtual	BRect				Bounds();

	virtual	void				RebuildCachedData();

 private:
			void				_SetTracking(uint32 mode);
			void				_BuildPropertyPath();

			BRect				_DirtyRectForKey(KeyFrame* key) const;
			BRect				_DirtyRectForKey(int32 keyIndex) const;

	SplitManipulator*			fParent;

	PropertyAnimator*			fAnimator;

	List<BPoint*>				fKeyPoints;
	uint64						fCachedClipOffset;
	uint64						fCachedDuration;

	uint32						fTracking;
	int64						fStartFrame;
	bool						fFrameFixed;
	int32						fKeyIndex;
	KeyFrame*					fDraggedKey;
	Command*					fCommand;
};

#endif // PROPERTY_MANIPULATOR_H

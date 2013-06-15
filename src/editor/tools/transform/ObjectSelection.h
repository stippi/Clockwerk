/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef OBJECT_SELECTION_H
#define OBJECT_SELECTION_H

#include <List.h>

#include "TransformBox.h"

class FloatProperty;
class PlaylistItem;
class PropertyObject;

class ObjectSelection : public TransformBox {
 public:
								ObjectSelection(StateView* view,
												PlaylistItem** objects,
												int32 count);
	virtual						~ObjectSelection();

	// Observer interface (Manipulator is an Observer)
	virtual	void				ObjectChanged(const Observable* object);

	// StageManipulator interface
	virtual	void				SetCurrentFrame(int64 frame);

	// TransformBox interface
	virtual	void				Update(bool deep = true);

	virtual	void				TransformFromCanvas(BPoint& point) const;
	virtual	void				TransformToCanvas(BPoint& point) const;
	virtual	double				ViewSpaceRotation() const;

	virtual	TransformCommand*	MakeAction(const char* actionName,
										   uint32 nameIndex) const;

	virtual	Command*			FinishTransaction();

	// ObjectSelection
			Command*			Perform();
			Command*			Cancel();

			PlaylistItem**		Objects() const
									{ return fObjects; }
			int32				CountObjects() const
									{ return fCount; }

 private:
			void				_SetProperty(PropertyObject* object,
											 FloatProperty* property,
											 float value, int64 frame);

			PlaylistItem**		fObjects;
			int32				fCount;

			// saves the transformable objects transformation states
			// prior to this transformation
			BPoint				fOriginalPivot;

			float*				fSavedPivotX;
			float*				fSavedPivotY;
			float*				fSavedTransationX;
			float*				fSavedTransationY;
			float*				fSavedRotation;
			float*				fSavedScaleX;
			float*				fSavedScaleY;

			AffineTransform		fParentTransform;

			int64				fCurrentFrame;
			bool				fIgnoreUpdates;

			BList				fKeyFrameCommands;
};

#endif // OBJECT_SELECTION_H


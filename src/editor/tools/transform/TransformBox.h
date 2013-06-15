/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TRANSFORM_BOX_H
#define TRANSFORM_BOX_H

#include "AdvancedTransform.h"
#include "StageManipulator.h"

class Command;
class StateView;
class DragState;
class TransformCommand;

class TransformBox : public AdvancedTransform,
					 public StageManipulator {
 public:
								TransformBox(StateView* view,
											 BRect box);
								TransformBox(const TransformBox& other);
	virtual						~TransformBox();

	// StageManipulator interface
	virtual	void				Draw(BView* into, BRect updateRect);

	virtual	bool				MouseDown(BPoint where);
	virtual	void				MouseMoved(BPoint where);
	virtual	Command*			MouseUp();
	virtual	bool				MouseOver(BPoint where);
	virtual	bool				DoubleClicked(BPoint where);

	virtual	BRect				Bounds();
	virtual	BRect				TrackingBounds(BView* withinView);

	virtual	void				AttachedToView(StateView* view);
	virtual	void				DetachedFromView(StateView* view);

	// TransformBox
			void				Set(const AffineTransform& t);

	virtual	void				Update(bool deep = true);

			BPoint				Origin() const;
				// TODO: rework entire pivot/origin/center stuff... :-(
			void				OffsetPivot(BPoint offset);
//			BPoint				CenterOffset() const
//									{ return fCenterOffset; }
			void				SetBox(BRect box);
			BRect				Box() const
									{ return fOriginalBox; }

	virtual	Command*			FinishTransaction();

			void				NudgeBy(BPoint offset);
			bool				IsNudging() const
									{ return fNudging; }
			Command*			FinishNudging();

	virtual	void				TransformFromCanvas(BPoint& point) const;
	virtual	void				TransformToCanvas(BPoint& point) const;


	virtual	void				ModifiersChanged(uint32 modifiers);

	virtual	bool				UpdateCursor();

	virtual	TransformCommand*	MakeAction(const char* actionName,
										   uint32 nameIndex) const = 0;

			bool				IsRotating() const
									{ return fCurrentState == fRotateState; }
	virtual	double				ViewSpaceRotation() const;

 private:
			void				_UpdateAffine();
			DragState*			_DragStateFor(BPoint canvasWhere,
											  float canvasZoom);
			void				_StrokeBWLine(BView* into,
											  BPoint from, BPoint to) const;
			void				_StrokeBWPoint(BView* into,
											   BPoint point, double angle) const;

			BRect				fOriginalBox;

			BPoint				fLeftTop;
			BPoint				fRightTop;
			BPoint				fLeftBottom;
			BPoint				fRightBottom;

			BPoint				fPivot;

			TransformCommand*	fCurrentCommand;
			DragState*			fCurrentState;

			bool				fDragging;
			BPoint				fMousePos;
			uint32				fModifiers;

			bool				fNudging;

 protected:
			// "static" state objects
			void				_SetState(DragState* state);

			StateView*			fView;

			DragState*			fDragLTState;
			DragState*			fDragRTState;
			DragState*			fDragLBState;
			DragState*			fDragRBState;

			DragState*			fDragLState;
			DragState*			fDragRState;
			DragState*			fDragTState;
			DragState*			fDragBState;

			DragState*			fRotateState;
			DragState*			fTranslateState;
			DragState*			fOffsetCenterState;
};

#endif // TRANSFORM_BOX_H

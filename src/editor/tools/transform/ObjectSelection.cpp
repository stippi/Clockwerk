/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ObjectSelection.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "AddKeyFrameCommand.h"
#include "CommonPropertyIDs.h"
#include "CompoundCommand.h"
#include "KeyFrame.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "StateView.h"
#include "TransformObjectsCommand.h"

using std::nothrow;

// constructor
ObjectSelection::ObjectSelection(StateView* view,
								 PlaylistItem** objects,
								 int32 count)
	: TransformBox(view, BRect(0.0, 0.0, 1.0, 1.0)),

	  fObjects(objects && count > 0 ? new PlaylistItem*[count] : NULL),
	  fCount(count),

	  fSavedPivotX(NULL),
	  fSavedPivotY(NULL),
	  fSavedTransationX(NULL),
	  fSavedTransationY(NULL),
	  fSavedRotation(NULL),
	  fSavedScaleX(NULL),
	  fSavedScaleY(NULL),

	  fParentTransform(),

	  fCurrentFrame(0),
	  fIgnoreUpdates(false),

	  fKeyFrameCommands(7)
{
	if (fObjects) {
		// allocate storage for the current transformations
		// of each object
		fSavedPivotX		= new float[fCount];
		fSavedPivotY		= new float[fCount];
		fSavedTransationX	= new float[fCount];
		fSavedTransationY	= new float[fCount];
		fSavedRotation		= new float[fCount];
		fSavedScaleX		= new float[fCount];
		fSavedScaleY		= new float[fCount];

		memcpy(fObjects, objects, fCount * sizeof(PlaylistItem*));
		for (int32 i = 0; i < fCount; i++) {
			if (fObjects[i])
				fObjects[i]->AddObserver(this);
		}

		// trigger init
		ObjectChanged(fObjects[0]);
	} else {
		SetBox(BRect(0, 0, -1, -1));
	}
}

// destructor
ObjectSelection::~ObjectSelection()
{
	for (int32 i = 0; i < fCount; i++) {
		if (fObjects[i])
			fObjects[i]->RemoveObserver(this);
	}
	delete[] fObjects;

	delete[] fSavedPivotX;
	delete[] fSavedPivotY;
	delete[] fSavedTransationX;
	delete[] fSavedTransationY;
	delete[] fSavedRotation;
	delete[] fSavedScaleX;
	delete[] fSavedScaleY;
}

// #pragma mark -

// ObjectChanged
void
ObjectSelection::ObjectChanged(const Observable* object)
{
	if (!fView->LockLooper())
		return;

	fIgnoreUpdates = true;

	fParentTransform.Reset();
	// figure out bounds and init transformations
	if (fCount == 1 && fObjects[0]) {
		fSavedPivotX[0]			= 0;
		fSavedPivotY[0]			= 0;
		fSavedTransationX[0]	= 0.0;
		fSavedTransationY[0]	= 0.0;
		fSavedRotation[0]		= 0.0;
		fSavedScaleX[0]			= 1.0;
		fSavedScaleY[0]			= 1.0;

		// TODO: use real canvas bounds instead of fView->Bounds()!
		BRect bounds = fObjects[0]->Bounds(fView->Bounds(), false);
//		bounds.OffsetBy(pivot);
		fOriginalPivot.x = 0;
		fOriginalPivot.y = 0;
		SetBox(bounds);

		SetTransformation(BPoint(fObjects[0]->PivotX()->Value(),
						  		 fObjects[0]->PivotY()->Value()),
						  BPoint(fObjects[0]->TranslationX()->Value(),
						  		 fObjects[0]->TranslationY()->Value()),
						  fObjects[0]->Rotation()->Value(),
						  fObjects[0]->ScaleX()->Value(),
						  fObjects[0]->ScaleY()->Value());
	} else {
		BRect box(0, 0, -1, -1);
		for (int32 i = 0; i < fCount; i++) {
			if (!fObjects[i])
				continue;

			// TODO: use real canvas bounds instead of fView->Bounds()!
			box = !box.IsValid() ? fObjects[i]->Bounds(fView->Bounds())
								 : box | fObjects[i]->Bounds(fView->Bounds());

			fSavedPivotX[i]			= fObjects[i]->PivotX()->Value();
			fSavedPivotY[i]			= fObjects[i]->PivotY()->Value();
			fSavedTransationX[i]	= fObjects[i]->TranslationX()->Value();
			fSavedTransationY[i]	= fObjects[i]->TranslationY()->Value();
			fSavedRotation[i]		= fObjects[i]->Rotation()->Value();
			fSavedScaleX[i]			= fObjects[i]->ScaleX()->Value();
			fSavedScaleY[i]			= fObjects[i]->ScaleY()->Value();
		}
		Reset();
		// update with the changed box
		fOriginalPivot.x = (box.left + box.right) / 2.0;
		fOriginalPivot.y = (box.top + box.bottom) / 2.0;
		SetPivot(fOriginalPivot);
		SetBox(box);
	}

	fIgnoreUpdates = false;

	fView->UnlockLooper();
}

// #pragma mark -

// SetCurrentFrame
void
ObjectSelection::SetCurrentFrame(int64 frame)
{
	fCurrentFrame = frame;
}

// #pragma mark -

// Update
void
ObjectSelection::Update(bool deep)
{
	BRect r = Bounds();

	TransformBox::Update(deep);

	BRect dirty(r | Bounds());
	dirty.InsetBy(-8, -8);
	fView->Invalidate(dirty);

	if (!deep || !fObjects || fIgnoreUpdates)
		return;

	BPoint pivotDiff(Pivot().x - fOriginalPivot.x,
					 Pivot().y - fOriginalPivot.y);

	// TODO: completely broken for multiple objects!

	for (int32 i = 0; i < fCount; i++) {
		if (!fObjects[i])
			continue;

		int64 frame = fCurrentFrame - fObjects[i]->StartFrame();

		fObjects[i]->RemoveObserver(this);
		fObjects[i]->SuspendNotifications(true);

		if (fCount == 1) {
			_SetProperty(fObjects[i], fObjects[i]->PivotX(),
						 fSavedPivotX[i] + pivotDiff.x, frame);
			_SetProperty(fObjects[i], fObjects[i]->PivotY(),
						 fSavedPivotY[i] + pivotDiff.y, frame);
		}

		_SetProperty(fObjects[i], fObjects[i]->TranslationX(),
					 fSavedTransationX[i] + Translation().x, frame);
		_SetProperty(fObjects[i], fObjects[i]->TranslationY(),
					 fSavedTransationY[i] + Translation().y, frame);

		_SetProperty(fObjects[i], fObjects[i]->Rotation(),
					 fSavedRotation[i] + LocalRotation(), frame);

		_SetProperty(fObjects[i], fObjects[i]->ScaleX(),
					 fSavedScaleX[i] * LocalXScale(), frame);
		_SetProperty(fObjects[i], fObjects[i]->ScaleY(),
					 fSavedScaleY[i] * LocalYScale(), frame);

		fObjects[i]->SuspendNotifications(false);
		fObjects[i]->AddObserver(this);
	}
}

// TransformFromCanvas
void
ObjectSelection::TransformFromCanvas(BPoint& point) const
{
	fParentTransform.InverseTransform(&point);
	TransformBox::TransformFromCanvas(point);
}

// TransformToCanvas
void
ObjectSelection::TransformToCanvas(BPoint& point) const
{
	TransformBox::TransformToCanvas(point);
	fParentTransform.Transform(&point);
}

// ViewSpaceRotation
double
ObjectSelection::ViewSpaceRotation() const
{
	AffineTransform t(*this);
	t.Multiply(fParentTransform);
	return t.rotation() * 180.0 / M_PI;
}

// MakeAction
TransformCommand*
ObjectSelection::MakeAction(const char* actionName, uint32 nameIndex) const
{
	return new TransformObjectsCommand(fObjects, fCount, fCurrentFrame,

									   Pivot(),
									   Translation(),
									   LocalRotation(),
									   LocalXScale(),
									   LocalYScale(),

									   actionName,
									   nameIndex);
}

// FinishTransaction
Command*
ObjectSelection::FinishTransaction()
{
	int32 count = fKeyFrameCommands.CountItems();
	if (count > 0) {
		Command** commands = new Command*[count + 1];
		for (int32 i = 0; i < count; i++)
			commands[i] = (Command*)fKeyFrameCommands.ItemAtFast(i);
		fKeyFrameCommands.MakeEmpty();

		Command* transformCommand = TransformBox::FinishTransaction();
		commands[count] = transformCommand;

		BString commandName;
		if (transformCommand)
			transformCommand->GetName(commandName);
		
		return new CompoundCommand(commands, count, commandName.String(), 0);
	} else {
		return TransformBox::FinishTransaction();
	}
}

// Perform
Command*
ObjectSelection::Perform()
{
	return NULL;
}

// Cancel
Command*
ObjectSelection::Cancel()
{
	SetTransformation(B_ORIGIN, B_ORIGIN, 0.0, 1.0, 1.0);

	return NULL;
}

// #pragma mark -

// _SetProperty
void
ObjectSelection::_SetProperty(PropertyObject* object, FloatProperty* property,
							  float value, int64 frame)
{
	if (PropertyAnimator* animator = property->Animator()) {
		// need to set value of keyframe property
		KeyFrame* key = animator->KeyFrameBeforeOrAt(frame);
		if (!key || key->Frame() < frame) {
			// need to insert keyframe
			key = animator->InsertKeyFrameAt(frame);
			// remember this in an AddKeyFramCommand
			AddKeyFrameCommand* command = new AddKeyFrameCommand(animator, key);
			fKeyFrameCommands.AddItem(command);
		}

		FloatProperty* f = dynamic_cast<FloatProperty*>(key->Property());
		if (f && f->SetValue(value))
			object->ValueChanged(property);
	} else {
		if (property->SetValue(value))
			object->ValueChanged(property);
	}
}





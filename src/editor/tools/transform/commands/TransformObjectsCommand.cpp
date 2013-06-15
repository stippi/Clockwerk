/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "TransformObjectsCommand.h"

#include <stdio.h>

#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"

// constructor
TransformObjectsCommand::TransformObjectsCommand(PlaylistItem** objects,
												 int32 count,
												 int64 frame,
												 
												 BPoint pivot,
												 BPoint translation,
												 double rotation,
												 double xScale,
												 double yScale,
												 
												 const char* actionName,
												 int32 actionIndex)
	: TransformCommand(pivot,
					   translation,
					   rotation,
					   xScale,
					   yScale,

					   actionName, actionIndex),

	  fObjects(objects ? new PlaylistItem*[count] : NULL),
	  fCount(count),
	  fFrame(frame)
{
	if (fObjects)
		memcpy(fObjects, objects, sizeof(PlaylistItem*) * fCount);
}

// destructor
TransformObjectsCommand::~TransformObjectsCommand()
{
	delete[] fObjects;
}

// InitCheck
status_t
TransformObjectsCommand::InitCheck()
{
	return fObjects ? TransformCommand::InitCheck()
					: B_NO_INIT;
}

// #pragma mark -

// _SetTransformation
status_t
TransformObjectsCommand::_SetTransformation(BPoint pivotDiff,
											BPoint translationDiff,
											double rotationDiff,
											double xScaleDiff,
											double yScaleDiff) const
{
	for (int32 i = 0; i < fCount; i++) {
		if (!fObjects[i])
			continue;

		fObjects[i]->SuspendNotifications(true);

		int64 frame = fFrame - fObjects[i]->StartFrame();

		// pivot
		_SetProperty(fObjects[i], fObjects[i]->PivotX(),
					 fObjects[i]->PivotX()->Value() + pivotDiff.x,
					 frame);
		_SetProperty(fObjects[i], fObjects[i]->PivotY(),
					 fObjects[i]->PivotY()->Value() + pivotDiff.y,
					 frame);

		// translation
		_SetProperty(fObjects[i], fObjects[i]->TranslationX(),
					 fObjects[i]->TranslationX()->Value()
							   + translationDiff.x,
					 frame);
		_SetProperty(fObjects[i], fObjects[i]->TranslationY(),
					 fObjects[i]->TranslationY()->Value()
							   + translationDiff.y,
					 frame);

		// rotation
		_SetProperty(fObjects[i], fObjects[i]->Rotation(),
					 fObjects[i]->Rotation()->Value() + rotationDiff,
					 frame);

		// scale
		_SetProperty(fObjects[i], fObjects[i]->ScaleX(),
					 fObjects[i]->ScaleX()->Value() + xScaleDiff,
					 frame);
		_SetProperty(fObjects[i], fObjects[i]->ScaleY(),
					 fObjects[i]->ScaleY()->Value() + yScaleDiff,
					 frame);


		fObjects[i]->SuspendNotifications(false);

	}

	return B_OK;
}

// _SetProperty
void
TransformObjectsCommand::_SetProperty(PropertyObject* object,
									  FloatProperty* property,
									  float value, int64 frame) const
{
	if (PropertyAnimator* animator = property->Animator()) {
		// need to set value of keyframe property
		KeyFrame* key = animator->KeyFrameBeforeOrAt(frame);
		if (!key || key->Frame() < frame) {
			printf("TransformObjectsCommand::_SetProperty() - "
				   "no keyframe at frame!\n");
			return;
		}

		FloatProperty* f = dynamic_cast<FloatProperty*>(key->Property());
		if (f && f->SetValue(value))
			object->ValueChanged(property);
	} else {
		if (property->SetValue(value))
			object->ValueChanged(property);
	}
}




/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ModifyKeyFrameCommand.h"

#include <stdio.h>

#include "KeyFrame.h"
#include "Property.h"
#include "PropertyAnimator.h"

// constructor
ModifyKeyFrameCommand::ModifyKeyFrameCommand(PropertyAnimator* animator,
											 KeyFrame* keyFrame)
	: Command(),
	  fAnimator(animator),
	  fKeyFrame(keyFrame),
	  fFrame(0),
	  fProperty(NULL)
{
	if (!fKeyFrame)
		return;

	fFrame = fKeyFrame->Frame();
	if (fKeyFrame->Property())
		fProperty = fKeyFrame->Property()->Clone(false);
}

// destructor
ModifyKeyFrameCommand::~ModifyKeyFrameCommand()
{
	delete fProperty;
}

// InitCheck
status_t
ModifyKeyFrameCommand::InitCheck()
{
	if (!fAnimator || !fKeyFrame)
		return B_NO_INIT;

	if (!fProperty)
		return B_NO_MEMORY;

	if (fKeyFrame->Property()->Equals(fProperty)
		&& fKeyFrame->Frame() == fFrame) {
		// no point in remembering the command
		return B_ERROR;
	}

	return B_OK;
}

// Perform
status_t
ModifyKeyFrameCommand::Perform()
{
	// the KeyFrame is already modified
	return B_OK;
}

// Undo
status_t
ModifyKeyFrameCommand::Undo()
{
	// toggle keyframe properties
	int64 oldFrame = fKeyFrame->Frame();
	Property* oldProperty = fKeyFrame->Property()->Clone(false);
	if (!oldProperty)
		return B_NO_MEMORY;

	AutoNotificationSuspender _(fAnimator);

	fKeyFrame->Property()->SetValue(fProperty);
	fAnimator->SetKeyFrameFrame(fKeyFrame, fFrame);

	// make sure we trigger a notification
	fAnimator->Notify();

	fFrame = oldFrame;
	delete fProperty;
	fProperty = oldProperty;

	return B_OK;
}

// Redo
status_t
ModifyKeyFrameCommand::Redo()
{
	return Undo();
}

// GetName
void
ModifyKeyFrameCommand::GetName(BString& name)
{
	name << "Change Key Frame";
}

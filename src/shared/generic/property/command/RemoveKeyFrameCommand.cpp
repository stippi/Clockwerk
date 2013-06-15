/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RemoveKeyFrameCommand.h"

#include <stdio.h>

#include "KeyFrame.h"
#include "PropertyAnimator.h"

// constructor
RemoveKeyFrameCommand::RemoveKeyFrameCommand(PropertyAnimator* animator,
											 KeyFrame* keyFrame)
	: Command(),
	  fAnimator(animator),
	  fKeyFrame(keyFrame),
	  fKeyFrameRemoved(true)
{
}

// destructor
RemoveKeyFrameCommand::~RemoveKeyFrameCommand()
{
	if (fKeyFrameRemoved) {
		// the KeyFrame belongs to us
		delete fKeyFrame;
	}
}

// InitCheck
status_t
RemoveKeyFrameCommand::InitCheck()
{
	if (fAnimator && fKeyFrame)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
RemoveKeyFrameCommand::Perform()
{
	// the KeyFrame is already removed
	return B_OK;
}

// Undo
status_t
RemoveKeyFrameCommand::Undo()
{
	if (fAnimator->AddKeyFrame(fKeyFrame)) {
		fKeyFrameRemoved = false;
		return B_OK;
	}
	return B_ERROR;
}

// Redo
status_t
RemoveKeyFrameCommand::Redo()
{
	if (fAnimator->RemoveKeyFrame(fKeyFrame)) {
		fKeyFrameRemoved = true;
		return B_OK;
	}
	return B_ERROR;
}

// GetName
void
RemoveKeyFrameCommand::GetName(BString& name)
{
	name << "Remove Key Frame";
}

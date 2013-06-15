/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "AddKeyFrameCommand.h"

#include <stdio.h>

#include "KeyFrame.h"
#include "PropertyAnimator.h"

// constructor
AddKeyFrameCommand::AddKeyFrameCommand(PropertyAnimator* animator,
									   KeyFrame* keyFrame)
	: Command(),
	  fAnimator(animator),
	  fKeyFrame(keyFrame),
	  fKeyFrameAdded(true)
{
}

// destructor
AddKeyFrameCommand::~AddKeyFrameCommand()
{
	if (!fKeyFrameAdded) {
		// the KeyFrame belong to us
		delete fKeyFrame;
	}
}

// InitCheck
status_t
AddKeyFrameCommand::InitCheck()
{
	if (fAnimator && fKeyFrame)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
AddKeyFrameCommand::Perform()
{
	// the KeyFrame is already added
	return B_OK;
}

// Undo
status_t
AddKeyFrameCommand::Undo()
{
	if (fAnimator->RemoveKeyFrame(fKeyFrame)) {
		fKeyFrameAdded = false;
		return B_OK;
	}
	return B_ERROR;
}

// Redo
status_t
AddKeyFrameCommand::Redo()
{
	if (fAnimator->AddKeyFrame(fKeyFrame)) {
		fKeyFrameAdded = true;
		return B_OK;
	}
	return B_ERROR;
}

// GetName
void
AddKeyFrameCommand::GetName(BString& name)
{
	name << "Insert Key Frame";
}

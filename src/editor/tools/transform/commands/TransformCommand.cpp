/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include "TransformCommand.h"

// constructor
TransformCommand::TransformCommand(BPoint pivot,
								   BPoint translation,
								   double rotation,
								   double xScale,
								   double yScale,
								   const char* actionName,
								   uint32 nameIndex)
	: Command(),
	  fOldPivot(pivot),
	  fOldTranslation(translation),
	  fOldRotation(rotation),
	  fOldXScale(xScale),
	  fOldYScale(yScale),

	  fNewPivot(pivot),
	  fNewTranslation(translation),
	  fNewRotation(rotation),
	  fNewXScale(xScale),
	  fNewYScale(yScale),

	  fName(actionName),
	  fNameIndex(nameIndex)
{
}

// constructor
TransformCommand::TransformCommand(const char* actionName,
								   uint32 nameIndex)
	: Command(),
	  fOldPivot(B_ORIGIN),
	  fOldTranslation(B_ORIGIN),
	  fOldRotation(0.0),
	  fOldXScale(1.0),
	  fOldYScale(1.0),

	  fNewPivot(B_ORIGIN),
	  fNewTranslation(B_ORIGIN),
	  fNewRotation(0.0),
	  fNewXScale(1.0),
	  fNewYScale(1.0),

	  fName(actionName),
	  fNameIndex(nameIndex)
{
}

// destructor
TransformCommand::~TransformCommand()
{
}

// InitCheck
status_t
TransformCommand::InitCheck()
{
	if ((fNewPivot != fOldPivot
		 || fNewTranslation != fOldTranslation
		 || fNewRotation != fOldRotation
		 || fNewXScale != fOldXScale
		 || fNewYScale != fOldYScale))
		return B_OK;
	else
		return B_NO_INIT;
}

// Perform
status_t
TransformCommand::Perform()
{
	// objects are already transformed
	return B_OK;
}

// Undo
status_t
TransformCommand::Undo()
{
	_SetTransformation(fOldPivot - fNewPivot,
					   fOldTranslation - fNewTranslation,
					   fOldRotation - fNewRotation,
					   fOldXScale - fNewXScale,
					   fOldYScale - fNewYScale);
	return B_OK;
}

// Redo
status_t
TransformCommand::Redo()
{
	_SetTransformation(fNewPivot - fOldPivot,
					   fNewTranslation - fOldTranslation,
					   fNewRotation - fOldRotation,
					   fNewXScale - fOldXScale,
					   fNewYScale - fOldYScale);
	return B_OK;
}

// GetName
void
TransformCommand::GetName(BString& name)
{
	name << _GetString(fNameIndex, fName.String());
}

// SetNewTransformation
void
TransformCommand::SetNewTransformation(BPoint pivot,
									   BPoint translation,
									   double rotation,
									   double xScale,
									   double yScale)
{
	fNewPivot = pivot;
	fNewTranslation = translation;
	fNewRotation = rotation;
	fNewXScale = xScale;
	fNewYScale = yScale;
}

// SetNewTranslation
void
TransformCommand::SetNewTranslation(BPoint translation)
{
	// TODO: what was this used for?!?
	fNewTranslation = translation;
}

// SetName
void
TransformCommand::SetName(const char* actionName, uint32 nameIndex)
{
	fName.SetTo(actionName);
	fNameIndex = nameIndex;
}


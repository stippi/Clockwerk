/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TrackingCommand.h"

#include "CurrentFrame.h"


// constructor
TrackingCommand::TrackingCommand()
	: Command()
	, fCurrentFrame(NULL)
	, fOriginalPlaybackPosition(0)
{
}

// destructor
TrackingCommand::~TrackingCommand()
{
}

// Perform
status_t
TrackingCommand::Perform()
{
	// action has already happened
	return B_OK;
}

// SetPlaybackManager
void
TrackingCommand::SetCurrentFrame(CurrentFrame* frame)
{
	if (fCurrentFrame == frame)
		return;

	ResetCurrentFrame();
	fCurrentFrame = frame;
	if (!fCurrentFrame)
		return;

	fOriginalPlaybackPosition = fCurrentFrame->VirtualFrame();
	fCurrentFrame->SetBeingDragged(true);
}

// SetCurrentFrame
void
TrackingCommand::SetCurrentFrame(int64 frame)
{
	if (!fCurrentFrame)
		return;

	fCurrentFrame->SetVirtualFrame(frame);
}

// ResetCurrentFrame
void
TrackingCommand::ResetCurrentFrame()
{
	if (!fCurrentFrame)
		return;

	fCurrentFrame->SetFrame(fOriginalPlaybackPosition);
	fCurrentFrame->SetBeingDragged(false);
}



/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REMOVE_KEY_FRAME_COMMAND_H
#define REMOVE_KEY_FRAME_COMMAND_H

#include "Command.h"

class PropertyAnimator;
class KeyFrame;

class RemoveKeyFrameCommand : public Command {
 public:
								RemoveKeyFrameCommand(
									PropertyAnimator* animator,
									KeyFrame* keyFrame);
	virtual						~RemoveKeyFrameCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			PropertyAnimator*	fAnimator;
			KeyFrame*			fKeyFrame;
			bool				fKeyFrameRemoved;
};

#endif // REMOVE_KEY_FRAME_COMMAND_H

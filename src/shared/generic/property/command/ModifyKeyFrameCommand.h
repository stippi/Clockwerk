/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef MODIFY_KEY_FRAME_COMMAND_H
#define MODIFY_KEY_FRAME_COMMAND_H

#include "Command.h"

class Property;
class PropertyAnimator;
class KeyFrame;

class ModifyKeyFrameCommand : public Command {
 public:
								ModifyKeyFrameCommand(
									PropertyAnimator* animator,
									KeyFrame* keyFrame);
	virtual						~ModifyKeyFrameCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

 private:
			PropertyAnimator*	fAnimator;
			KeyFrame*			fKeyFrame;
			int64				fFrame;
			Property*			fProperty;
};

#endif // MODIFY_KEY_FRAME_COMMAND_H

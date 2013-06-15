/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIME_RANGE_PANEL_H
#define TIME_RANGE_PANEL_H

#include "Panel.h"

class BButton;
class BTextControl;

class TimeRangePanel : public Panel {
 public:
								TimeRangePanel(BRect parentWindowFrame,
									int64 startFrame, int64 endFrame,
									BHandler* target, BMessage* message);
	virtual						~TimeRangePanel();

	// BWindow interface
	virtual	bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

 private:
			BTextControl*		fStartTimeTC;
			BTextControl*		fEndTimeTC;

			BButton*			fOkB;
			BButton*			fCancelB;

			BHandler*			fTarget;
			BMessage*			fMessage;
};

#endif // TIME_RANGE_PANEL_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INIT_PROGRESS_PANEL_H
#define INIT_PROGRESS_PANEL_H


#include <Window.h>

#include "ProgressReporter.h"


class BStatusBar;


class InitProgressPanel : public BWindow, public ProgressReporter {
public:
								InitProgressPanel();
	virtual						~InitProgressPanel();

	virtual	void				MessageReceived(BMessage* message);

	// ProgressReporter interface
	virtual	void				SetProgressTitle(const char* title);
	virtual	void				ReportProgress(float percentCompleted,
									const char* label = NULL);

private:
			BStatusBar*			fStatusBar;
};

#endif // INIT_PROGRESS_PANEL_H

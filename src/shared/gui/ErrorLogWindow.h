/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ERROR_LOG_WINDOW_H
#define ERROR_LOG_WINDOW_H

#include <String.h>

#include <Window.h>

#include "StatusOutput.h"

class StatusOutputView;

class ErrorLogWindow : public BWindow {
public:
								ErrorLogWindow(BRect frame,
									const char* initialMessage = NULL);
	virtual						~ErrorLogWindow();

	virtual	bool				QuitRequested();

	virtual void				MessageReceived(BMessage *message);

			::StatusOutput*		StatusOutput()
									{ return &fMessageStatusOutput; }

private:
			BString				_GetTimeString();
			void				_GetCurrentTime();
			void				_GetCurrentDate();

	StatusOutputView*			fStatusView;
	MessageStatusOutput			fMessageStatusOutput;

	time_t						fTime;

	char						fTimeStr[64];
	char						fDateStr[64];
	bigtime_t					fLastCheck;
};

#endif // ERROR_LOG_WINDOW_H

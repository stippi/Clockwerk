/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ErrorLogWindow.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <Message.h>

#include "StatusOutputView.h"


// message what codes
enum {
	MSG_PRINT_STATUS	= 'prts',
};


// constructor
ErrorLogWindow::ErrorLogWindow(BRect frame, const char* initialMessage)
	:
	BWindow(frame, "Error Log", B_TITLED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fStatusView(NULL),
	fMessageStatusOutput(this, MSG_PRINT_STATUS),
	fTime(time(NULL)),
	fLastCheck(0LL)
{
	fStatusView = new StatusOutputView(initialMessage);
	BButton* closeButton = new BButton("Close",
		new BMessage(B_QUIT_REQUESTED));

	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fStatusView)
		.AddStrut(5)
		.Add(BGroupLayoutBuilder(B_HORIZONTAL)
			.AddGlue()
			.Add(closeButton)
		)
	);
}

// destructor
ErrorLogWindow::~ErrorLogWindow()
{
}

// QuitRequested
bool
ErrorLogWindow::QuitRequested()
{
	Hide();
	return false;
}

// MessageReceived
void
ErrorLogWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PRINT_STATUS:
		{
			int32 type;
			const char* messageText;
			if (message->FindInt32("type", &type) == B_OK
				&& message->FindString("message", &messageText) == B_OK) {

				BString errorText("[");
				errorText << _GetTimeString() << "]  " << messageText;
				
				fStatusView->PrintStatusMessage(type, errorText.String());
			}
		}
		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// #pragma mark -

// _GetTimeString
BString
ErrorLogWindow::_GetTimeString()
{
	bigtime_t now = system_time();
	if (fLastCheck + 1000000 < now) {
		fTime = time(NULL);

		_GetCurrentTime();
		_GetCurrentDate();
		fLastCheck = now;
	}

	BString clockText;
	clockText << fDateStr << ", " << fTimeStr;
	return clockText;
}

// _GetCurrentTime
void
ErrorLogWindow::_GetCurrentTime()
{
	tm time = *localtime(&fTime);
	strftime(fTimeStr, 64, "%H:%M:%S", &time);
}

static const char *kShortEuroDateFormat = "%d.%m.%y";
//static const char *kLongEuroDateFormat = "%a, %d %B, %Y";

// _GetCurrentTime
void
ErrorLogWindow::_GetCurrentDate()
{
	tm time = *localtime(&fTime);
	strftime(fDateStr, 64, kShortEuroDateFormat, &time);
}

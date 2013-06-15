/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InitProgressPanel.h"

#include <Message.h>

#include <Box.h>
#include <GroupLayoutBuilder.h>
#include <Screen.h>
#include <StatusBar.h>


enum {
	MSG_SET_PROGRESS_TITLE		= 'stpt',
	MSG_SET_PROGRESS			= 'stpr'
};


// constructor
InitProgressPanel::InitProgressPanel()
	:
	BWindow(BRect(0, 0, 200, 100), "Clockwerk Init Progress",
		B_BORDERED_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS)
{
	fStatusBar = new BStatusBar("init status",
		"Starting Clockwerk"B_UTF8_ELLIPSIS);

	SetLayout(new BGroupLayout(B_HORIZONTAL));

	const float spacing = 8.0f;

	BGroupView* groupView = new BGroupView(B_VERTICAL, 0);
	BGroupLayoutBuilder(groupView)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(fStatusBar)
	;
	AddChild(new BBox(B_PLAIN_BORDER, groupView));

	BScreen screen(this);
	BRect screenFrame = screen.Frame();
	MoveTo((screenFrame.left + screenFrame.right - Frame().Width()) / 2.0,
		(screenFrame.top + screenFrame.bottom - Frame().Height()) / 2.0);
}

// destructor
InitProgressPanel::~InitProgressPanel()
{
}

// MessageReceived
void
InitProgressPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_PROGRESS_TITLE: {
			const char* title;
			if (message->FindString("title", &title) == B_OK)
				fStatusBar->Reset(title);
			break;
		}
		case MSG_SET_PROGRESS: {
			float percentCompleted;
			const char* trailingLabel;
			if (message->FindString("label", &trailingLabel) < B_OK)
				trailingLabel = NULL;
			if (message->FindFloat("progress", &percentCompleted) == B_OK)
				fStatusBar->Update(percentCompleted
					- fStatusBar->CurrentValue(), trailingLabel);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// #pragma mark -

// SetProgressTitle
void
InitProgressPanel::SetProgressTitle(const char* title)
{
	BMessage message(MSG_SET_PROGRESS_TITLE);
	message.AddString("title", title);
	PostMessage(&message, this);
}

// ReportProgress
void
InitProgressPanel::ReportProgress(float percentCompleted, const char* label)
{
	BMessage message(MSG_SET_PROGRESS);
	message.AddFloat("progress", percentCompleted);
	if (label)
		message.AddString("label", label);
	PostMessage(&message, this);
}


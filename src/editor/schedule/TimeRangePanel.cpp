/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TimeRangePanel.h"

#include <Box.h>
#include <Button.h>
#include <Message.h>
#include <Messenger.h>
#include <TextControl.h>

#include "support_date.h"

enum {
	MSG_OK		= 'okok',
	MSG_CANCEL	= 'cncl'
};

// constructor
TimeRangePanel::TimeRangePanel(BRect parentWindowFrame, int64 startFrame,
		int64 endFrame, BHandler* target, BMessage* message)
	: Panel(BRect(0, 0, 200, 50), "Render Schedule", B_TITLED_WINDOW_LOOK,
			B_NORMAL_WINDOW_FEEL, B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE)
	, fTarget(target)
	, fMessage(message)
{
	BRect frame = Bounds();

	BBox* box1 = new BBox(frame, "bg 1", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);

	frame = box1->Bounds();
	frame.InsetBy(5, 5);
	frame.bottom = frame.top + 15;

	fStartTimeTC = new BTextControl(frame, "start time", "Start Time",
		"", NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	float width, height;
	fStartTimeTC->GetPreferredSize(&width, &height);
	fStartTimeTC->ResizeTo(frame.Width(), height);
	box1->AddChild(fStartTimeTC);

	frame.OffsetTo(fStartTimeTC->Frame().LeftBottom() + BPoint(0, 5));
	fEndTimeTC = new BTextControl(frame, "end time", "End Time",
		"", NULL, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	fEndTimeTC->ResizeTo(frame.Width(), height);
	box1->AddChild(fEndTimeTC);

	frame = box1->Bounds();
	frame.bottom = fEndTimeTC->Frame().bottom + 5;

	box1->ResizeTo(frame.Width(), frame.Height());

	frame.OffsetTo(frame.LeftBottom() + BPoint(0, 1));

	BBox* box2 = new BBox(frame, "bg 2", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM,
		B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE_JUMP, B_PLAIN_BORDER);

	frame = box2->Bounds();
	frame.InsetBy(5, 10);
	frame.bottom = frame.top + 15;

	fOkB = new BButton(frame, "ok", "Render"B_UTF8_ELLIPSIS,
		new BMessage(MSG_OK), B_FOLLOW_RIGHT | B_FOLLOW_TOP);

	fOkB->GetPreferredSize(&width, &height);
	fOkB->ResizeTo(width, height);
	fOkB->MoveTo(frame.right - 5 - width, frame.top);
	box2->AddChild(fOkB);

	fCancelB = new BButton(frame, "cancel", "Cancel",
		new BMessage(MSG_CANCEL), B_FOLLOW_RIGHT | B_FOLLOW_TOP);

	fCancelB->GetPreferredSize(&width, &height);
	fCancelB->ResizeTo(width, height);
	fCancelB->MoveTo(fOkB->Frame().left - 15 - width, frame.top);
	box2->AddChild(fCancelB);

	frame = box2->Bounds();
	frame.bottom = fOkB->Frame().bottom + 10;

	box2->ResizeTo(frame.Width(), frame.Height());

	frame.left = 0;
	frame.top = 0;
	frame.right = box2->Frame().right;
	frame.bottom = box2->Frame().bottom;

	ResizeTo(frame.Width(), frame.Height());

	AddChild(box1);
	AddChild(box2);

	SetDefaultButton(fOkB);

	MoveTo((parentWindowFrame.left + parentWindowFrame.right
		- frame.Width()) / 2,
		(parentWindowFrame.top + parentWindowFrame.bottom
		- frame.Height()) / 2);

	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = frame.Width();
	minHeight = frame.Height(),
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	BString timeCodeString = string_for_frame(startFrame);
	fStartTimeTC->SetText(timeCodeString.String());
	timeCodeString = string_for_frame(endFrame);
	fEndTimeTC->SetText(timeCodeString.String());
}

// destructor
TimeRangePanel::~TimeRangePanel()
{
	delete fMessage;
}

// QuitRequested
bool
TimeRangePanel::QuitRequested()
{
	if (fMessage) {
		BMessage copy(*fMessage);
		copy.AddBool("cancel", true);
		return BMessenger(fTarget).SendMessage(&copy) == B_OK ? false : true;
	}
	return true;
}

// MessageReceived
void
TimeRangePanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_OK:
			if (fMessage) {
				BMessage copy(*fMessage);
				copy.AddString("start time", fStartTimeTC->Text());
				copy.AddString("end time", fEndTimeTC->Text());
				BMessenger(fTarget).SendMessage(&copy);
			}
			Cancel();
			break;

		case MSG_CANCEL:
			Cancel();
			break;

		default:
			Panel::MessageReceived(message);
			break;
	}
}

/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "StatusOutputView.h"

#include <string.h>

#include <Looper.h>
#include <ScrollView.h>
#include <TextView.h>

#include "AutoLocker.h"

// StatusOutputView
StatusOutputView::StatusOutputView(const char* initialMessage)
	:
	BGroupView(B_HORIZONTAL)
{
	fTextView = new BTextView("status text view");
	fTextView->SetStylable(true);
	fTextView->MakeEditable(false);
	fTextView->SetViewColor(255, 255, 255);
	fScrollView = new BScrollView("status output scroll view",
		fTextView, 0, false, true);
	AddChild(fScrollView);

	if (initialMessage)
		InternalPrintStatusMessage(STATUS_MESSAGE_INFO, initialMessage);
}

// InternalPrintStatusMessage
void
StatusOutputView::InternalPrintStatusMessage(uint32 type, const char* format,
	va_list list)
{
	StatusOutput::InternalPrintStatusMessage(type, format, list);
}

// InternalPrintStatusMessage
void
StatusOutputView::InternalPrintStatusMessage(uint32 type, const char* message)
{
	AutoLocker<BLooper> _(Looper());

	rgb_color color;
	switch (type) {
		case STATUS_MESSAGE_WARNING:
			color = (rgb_color){200, 127, 0, 255};
			break;
		case STATUS_MESSAGE_ERROR:
			color = (rgb_color){200, 0, 0, 255};
			break;
		case STATUS_MESSAGE_INFO:
		default:
			color = (rgb_color){0, 0, 0, 255};
			break;
	}

	text_run_array textRuns = {
		1,
		{
			{ 0, *be_fixed_font, color }
		}
	};

	int32 insertLength = strlen(message);
	int32 currentTotalLength = fTextView->TextLength();

	bool scrollToEnd = true;
	if (BScrollBar* scrollBar = fTextView->ScrollBar(B_VERTICAL)) {
		float min, max;
		scrollBar->GetRange(&min, &max);
		if (max - scrollBar->Value() > 5.0)
			scrollToEnd = false;
	}

	fTextView->Insert(currentTotalLength, message, insertLength, &textRuns);
	currentTotalLength += insertLength;

	if (scrollToEnd)
		fTextView->ScrollToOffset(currentTotalLength);
}

// Clear
void
StatusOutputView::Clear(const char* initialMessage)
{
	fTextView->SetText("");

	if (initialMessage)
		InternalPrintStatusMessage(STATUS_MESSAGE_INFO, initialMessage);
}

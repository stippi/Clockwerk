/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "UploadSelectionPanel.h"

#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <Screen.h>

#include <GroupLayoutBuilder.h>
#include <Box.h>
#include <Button.h>
#include <StringView.h>

#include "Uploader.h"
#include "UploadObjectListView.h"
#include "ScrollView.h"


enum {
	MSG_UPLOAD		= 'upld',
	MSG_CANCEL		= 'cncl'
};


// constructor
UploadSelectionPanel::UploadSelectionPanel(BMessage& settings, BWindow* window)
	:
	BWindow(BRect(50, 50, 600, 400), "Upload Confirmation",
		B_FLOATING_WINDOW_LOOK,
		window ? B_FLOATING_SUBSET_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE),

	fExitSemaphore(create_sem(0, "UploadSelectionPanel exit")),
	fReturnCode(RETURN_CANCEL),
	fWindow(window),
	fSettings(settings)
{
	fInfoView = new BStringView("", "");

	fUploadB = new BButton("Upload", new BMessage(MSG_UPLOAD));
	fCancelB = new BButton("Cancel", new BMessage(MSG_CANCEL));

	fUploadListView = new UploadObjectListView("upload listview");

	ScrollView* scrollView = new ScrollView(fUploadListView,
		SCROLL_VERTICAL | SCROLL_HORIZONTAL_MAGIC
			| SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS,
		"upload scrollview");

	AddChild(new BBox(B_PLAIN_BORDER,
		BGroupLayoutBuilder(B_VERTICAL)
			.Add(scrollView)
			.AddStrut(5)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL)
				.Add(fInfoView)
				.AddGlue(15)
				.Add(fCancelB)
				.AddStrut(10)
				.Add(fUploadB)
			)
		)
	);

	SetDefaultButton(fUploadB);

	if (fWindow)
		AddToSubset(fWindow);
}

// destructor
UploadSelectionPanel::~UploadSelectionPanel()
{
	delete_sem(fExitSemaphore);
}

// QuitRequested
bool
UploadSelectionPanel::QuitRequested()
{
	release_sem(fExitSemaphore);
	return false;
}

// MessageReceived
void
UploadSelectionPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_UPLOAD:
			fReturnCode = RETURN_OK;
			release_sem(fExitSemaphore);

		case MSG_CANCEL:
			release_sem(fExitSemaphore);

		default:
			BWindow::MessageReceived(message);
	}
}

// ListViewUploadVisitor
class ListViewUploadVisitor : public Uploader::UploadVisitor {
public:
	ListViewUploadVisitor(UploadObjectListView* listView, int32& objectCount)
		:
		fListView(listView),
		fObjectCount(objectCount)
	{
		fObjectCount = 0;
	}

	~ListViewUploadVisitor()
	{
		fListView->Sort();
	}

	virtual	void Visit(ServerObject* object, int32 level)
	{
		fListView->AddObject(object);
		fObjectCount++;
	}

private:
	UploadObjectListView*	fListView;
	int32&					fObjectCount;
};

// InitFromUploader
status_t
UploadSelectionPanel::InitFromUploader(const Uploader* uploader)
{
	if (!uploader)
		return B_BAD_VALUE;

	int32 objectCount;
	ListViewUploadVisitor visitor(fUploadListView, objectCount);
	status_t ret = uploader->VisitUploads(&visitor);
	if (ret < B_OK)
		return ret;

	BString text("Ready to commit ");
	text << objectCount << " objects";
	fInfoView->SetText(text.String());

	return B_OK;
}

// Go
UploadSelectionPanel::return_code
UploadSelectionPanel::Go(bool centerInParentFrame)
{
	// run the window thread, to get an initial layout of the controls
	Hide();
	Show();
	if (!Lock())
		return RETURN_CANCEL;

	const char* settingsKey = "upload selection panel";
	BMessage settings;
	if (fSettings.FindMessage(settingsKey, &settings) == B_OK) {
		BRect frame;
		if (settings.FindRect("window frame", &frame) == B_OK) {
			MoveTo(frame.LeftTop());
			ResizeTo(frame.Width(), frame.Height());
		}
		fUploadListView->RestoreSettings(&settings, "upload lv settings");
	} else if (centerInParentFrame) {
		// center the panel above the parent window
		BRect frame = Frame();
		BRect parentFrame;
		if (fWindow && fWindow->Lock()) {
			parentFrame = fWindow->Frame();
			fWindow->Unlock();
		} else {
			BScreen screen(this);
			parentFrame = screen.Frame();
		}

		MoveTo((parentFrame.left + parentFrame.right - frame.Width()) / 2.0,
			(parentFrame.top + parentFrame.bottom - frame.Height()) / 2.0);
	}

	Show();
	Unlock();

	// block this thread now, but keep the window repainting
	while (true) {
		status_t err = acquire_sem_etc(fExitSemaphore, 1,
			B_CAN_INTERRUPT | B_RELATIVE_TIMEOUT, 50000);
		if (err != B_TIMED_OUT && err != B_INTERRUPTED)
			break;
		if (fWindow && fWindow->IsLocked())
			fWindow->UpdateIfNeeded();
	}

	if (!Lock())
		return RETURN_CANCEL;

	// store settings
	if (settings.ReplaceRect("window frame", Frame()) < B_OK)
		settings.AddRect("window frame", Frame());
	fUploadListView->StoreSettings(&settings, "upload lv settings");
	if (fSettings.ReplaceMessage(settingsKey, &settings) < B_OK)
		fSettings.AddMessage(settingsKey, &settings);

	return_code code = fReturnCode;
	Quit();

	return code;
}


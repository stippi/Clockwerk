/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "NetworkStatusPanel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Message.h>

#include <Box.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <TextControl.h>

#include "JobConnection.h"
#include "StatusOutputView.h"


// message what codes
enum {
	MSG_PRINT_STATUS	= 'prts',
	MSG_INVOKE_BUTTON	= 'inkb',
};


// constructor
NetworkStatusPanel::NetworkStatusPanel(BRect frame,
		BWindow* window, BHandler* target, BMessage* message,
		JobConnection* connection, const char* initialMessage)
	:
	BWindow(frame, "Network Status", B_FLOATING_WINDOW_LOOK,
		B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS),
	fConnection(NULL),
	fMessageStatusOutput(this, MSG_PRINT_STATUS),
	fWindow(window),
	fTarget(target),
	fMessage(message),
	fCancel(false)
{
	fServerAddressTC = new BTextControl("Server", "", NULL);
	fClientIDTC = new BTextControl("Client ID", "", NULL);

	fStatusView = new StatusOutputView(initialMessage);

	fConnectB = new BButton("Connect", new BMessage(MSG_NETWORK_CONNECT));
	fDisconnectB = new BButton("Disconnect",
		new BMessage(MSG_NETWORK_DISCONNECT));
	fUpdateB = new BButton("Update", new BMessage(MSG_NETWORK_UPDATE));
	fCommitB = new BButton("Commit", new BMessage(MSG_NETWORK_COMMIT));
	fStopB = new BButton("Stop", new BMessage(MSG_NETWORK_STOP));

	fHideB = new BButton("Hide", new BMessage(MSG_INVOKE_BUTTON));

	AddChild(
		new BBox(B_PLAIN_BORDER, BGroupLayoutBuilder(B_VERTICAL, 5)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL, 5)
				.Add(fServerAddressTC)
				.Add(fClientIDTC)
			)
			.Add(fStatusView)
			.Add(BGroupLayoutBuilder(B_HORIZONTAL)
				.Add(fConnectB)
				.AddStrut(20)
				.Add(fDisconnectB)
				.AddGlue()
				.Add(fUpdateB)
				.AddStrut(30)
				.Add(fCommitB)
				.AddStrut(30)
				.Add(fStopB)
				.AddGlue()
				.Add(fHideB)
			)
		)
	);

	fConnectB->SetTarget(be_app);
	fDisconnectB->SetTarget(be_app);
	fUpdateB->SetTarget(be_app);
	fCommitB->SetTarget(be_app);
	fStopB->SetTarget(be_app);

	SetConnection(connection);

	fConnectB->SetEnabled(true);
	fDisconnectB->SetEnabled(false);
	fUpdateB->SetEnabled(false);
	fCommitB->SetEnabled(false);
	fStopB->SetEnabled(false);
	fServerAddressTC->SetEnabled(false);
	fClientIDTC->SetEnabled(false);

	if (fWindow)
		AddToSubset(fWindow);
}

// destructor
NetworkStatusPanel::~NetworkStatusPanel()
{
	SetConnection(NULL);
	delete fMessage;
}

// QuitRequested
bool
NetworkStatusPanel::QuitRequested()
{
	if (!fCancel)
		_SendMessage();

	if (!IsHidden())
		Hide();

	return false;
}

// MessageReceived
void
NetworkStatusPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_PRINT_STATUS: {
			int32 type;
			const char* messageText;
			if (message->FindInt32("type", &type) == B_OK
				&& message->FindString("message", &messageText) == B_OK) {
				fStatusView->PrintStatusMessage(type, messageText);
			}
			break;
		}
		case MSG_INVOKE_BUTTON:
			_SendMessage();
			break;

		case MSG_CONNECTING:
			fConnectB->SetEnabled(false);
			fDisconnectB->SetEnabled(false);
			fUpdateB->SetEnabled(false);
			fCommitB->SetEnabled(false);
			fStopB->SetEnabled(true);
			break;
		case MSG_DISCONNECTED:
			fConnectB->SetEnabled(true);
			fDisconnectB->SetEnabled(false);
			fUpdateB->SetEnabled(false);
			fCommitB->SetEnabled(false);
			fStopB->SetEnabled(false);
			break;

		case MSG_UPDATING:
		case MSG_COMMITING:
			fConnectB->SetEnabled(false);
			fDisconnectB->SetEnabled(false);
			fCommitB->SetEnabled(false);
			fUpdateB->SetEnabled(false);
			fStopB->SetEnabled(true);
			break;
		case MSG_CONNECTED:
		case MSG_NETWORK_JOB_DONE:
			fConnectB->SetEnabled(false);
			fDisconnectB->SetEnabled(true);
			fUpdateB->SetEnabled(true);
			fCommitB->SetEnabled(true);
			fStopB->SetEnabled(false);
			break;

		default:
			BWindow::MessageReceived(message);
			break;
	}
}

// Show
void
NetworkStatusPanel::Show()
{
	if (fWindow && fWindow->Lock()) {
		fSavedTargetWindowFeel = fWindow->Feel();
		if (fSavedTargetWindowFeel != B_NORMAL_WINDOW_FEEL)
			fWindow->SetFeel(B_NORMAL_WINDOW_FEEL);
		fWindow->Unlock();
	}	

	BWindow::Show();
}

// Hide
void
NetworkStatusPanel::Hide()
{
	BWindow::Hide();

	if (fWindow && fWindow->Lock()) {
		fWindow->SetFeel(fSavedTargetWindowFeel);
		fWindow->Unlock();
	}	
}

// #pragma mark -

// StatusOutput
::StatusOutput*
NetworkStatusPanel::StatusOutput()
{
	return &fMessageStatusOutput;
}

// SetConnection
void
NetworkStatusPanel::SetConnection(JobConnection* connection)
{
	if (fConnection == connection)
		return;

// TODO: this will block on the status output lock
// of the connection, which is not released in case
// a job is stalled, since we are not deleted as
// long as any connection is alive, it shouldn't be
// a problem if we don't set the status output NULL
//	if (fConnection)
//		fConnection->SetStatusOutput(NULL);

	fConnection = connection;

	if (fConnection)
		fConnection->SetStatusOutput(&fMessageStatusOutput);
}

// SetServerAndClientID
void
NetworkStatusPanel::SetServerAndClientID(const char* server,
	const char* clientID)
{
	if (!Lock())
		return;

	fServerAddressTC->SetText(server);
	fClientIDTC->SetText(clientID);

	Unlock();
}


// SetLabelAndMessage
void
NetworkStatusPanel::SetLabelAndMessage(const char* label, BMessage* message,
	bool cancel)
{
	if (!Lock())
		return;

	delete fMessage;
	fMessage = message;

	fHideB->SetLabel(label);
	fHideB->SetEnabled(true);
	fCancel = cancel;

	Unlock();
}
									
// ClearOutput
void
NetworkStatusPanel::ClearOutput(const char* initialMessage)
{
	if (Lock()) {
		fStatusView->Clear(initialMessage);
		Unlock();
	}
}

// #pragma mark -

// _SendMessage
void
NetworkStatusPanel::_SendMessage()
{
	if (!fTarget)
		fTarget = fWindow;
	BLooper* looper = fTarget ? fTarget->Looper() : NULL;
	if (fMessage && looper)
		looper->PostMessage(fMessage, fTarget);

	if (fCancel) {
		fStatusView->PrintInfoMessage("Shutting down connection - please wait"B_UTF8_ELLIPSIS"\n");
		fHideB->SetEnabled(false);
	}
}


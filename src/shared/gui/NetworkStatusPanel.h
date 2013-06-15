/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NETWORK_STATUS_PANEL_H
#define NETWORK_STATUS_PANEL_H

#include <Window.h>

#include "Panel.h"
#include "StatusOutput.h"

class BButton;
class BTextControl;
class JobConnection;
class StatusOutputView;


enum {
	MSG_NETWORK_CONNECT				= 'nwcn',
	MSG_NETWORK_DISCONNECT			= 'nwdc',

	MSG_NETWORK_UPDATE				= 'nwup',
	MSG_NETWORK_COMMIT				= 'nwcm',

	MSG_NETWORK_STOP				= 'nwsp',

	MSG_UPDATING					= 'updg',
	MSG_COMMITING					= 'cmtg',
	MSG_NETWORK_JOB_DONE			= 'nwjd',
	MSG_CONNECTING					= 'cnng',
	MSG_CONNECTED					= 'cnnt',
	MSG_DISCONNECTED				= 'dsct',
};


class NetworkStatusPanel : public BWindow {
public:
								NetworkStatusPanel(
									BRect frame,
									BWindow* window,
									BHandler* target,
									BMessage* message,
									JobConnection* connection,
									const char* initialMessage = NULL);
	virtual						~NetworkStatusPanel();

	virtual	bool				QuitRequested();

	virtual void				MessageReceived(BMessage *message);

	virtual	void				Show();
	virtual	void				Hide();

	// NetworkStatusPanel
			::StatusOutput*		StatusOutput();

			void				SetConnection(JobConnection* connection);
			void				SetLabelAndMessage(const char* label,
									BMessage* message, bool cancel);
			void				ClearOutput(const char* initialMessage = NULL);
			void				SetServerAndClientID(const char* server,
									const char* clientID);

private:
			void				_SendMessage();

	BTextControl*				fServerAddressTC;
	BTextControl*				fClientIDTC;

	StatusOutputView*			fStatusView;

	BButton*					fConnectB;
	BButton*					fDisconnectB;
	BButton*					fUpdateB;
	BButton*					fCommitB;
	BButton*					fStopB;
	BButton*					fHideB;

	JobConnection*				fConnection;
	MessageStatusOutput			fMessageStatusOutput;

	BWindow*					fWindow;
	BHandler*					fTarget;
	BMessage*					fMessage;
	bool						fCancel;

	window_feel					fSavedTargetWindowFeel;
};

#endif // NETWORK_STATUS_PANEL_H

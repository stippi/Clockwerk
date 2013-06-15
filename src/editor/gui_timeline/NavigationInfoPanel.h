/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NAVIGATION_INFO_PANEL_H
#define NAVIGATION_INFO_PANEL_H

#include <Message.h>
#include <Messenger.h>
#include <Window.h>

class BStringView;
class BTextControl;

class NavigationInfoPanel : public BWindow {
public:
								NavigationInfoPanel(BWindow* parent,
									const BMessage& message,
									const BMessenger& target);
	virtual						~NavigationInfoPanel();

	virtual	bool				QuitRequested();
	virtual	void				MessageReceived(BMessage* message);

			void				SetLabel(const char* text);
			void				SetTargetID(const char* targetID);
			void				SetMessage(const BMessage& message);

private:
			void				_Invoke();

	class InfoView;

			InfoView*			fInfoView;
			BStringView*		fLabelView;
			BTextControl*		fTargetClipTC;
			BMessage			fMessage;
			BMessenger			fTarget;
};

#endif // NAVIGATION_INFO_PANEL_H

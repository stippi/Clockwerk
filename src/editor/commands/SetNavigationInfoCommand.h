/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SET_NAVIGATION_INFO_COMMAND_H
#define SET_NAVIGATION_INFO_COMMAND_H

#include "Command.h"

class PlaylistItem;
class NavigationInfo;

class SetNavigationInfoCommand : public Command {
 public:
								SetNavigationInfoCommand(
									PlaylistItem* item,
									const NavigationInfo* newInfo);
	virtual						~SetNavigationInfoCommand();
	
	// Command Interface
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();

	virtual void				GetName(BString& name);

 private:
			PlaylistItem*		fItem;
			NavigationInfo*		fNewInfo;
			NavigationInfo*		fOldInfo;
};

#endif // SET_NAVIGATION_INFO_COMMAND_H

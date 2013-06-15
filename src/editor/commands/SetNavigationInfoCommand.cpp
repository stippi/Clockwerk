/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SetNavigationInfoCommand.h"

#include <new>
#include <stdio.h>

#include "PlaylistItem.h"
#include "NavigationInfo.h"

using std::nothrow;

// constructor
SetNavigationInfoCommand::SetNavigationInfoCommand(PlaylistItem* item,
		const NavigationInfo* newInfo)
	: Command()
	, fItem(item)
	, fNewInfo(NULL)
	, fOldInfo(NULL)
{
	if (!fItem)
		return;

	if (newInfo) {
		fNewInfo = new (nothrow) NavigationInfo(*newInfo);
		if (!fNewInfo) {
			fItem = NULL;
			return;
		}
	}

	if (const NavigationInfo* info = fItem->NavigationInfo()) {
		fOldInfo = new (nothrow) NavigationInfo(*info);
		if (!fOldInfo) {
			fItem = NULL;
			return;
		}
	}
}

// destructor
SetNavigationInfoCommand::~SetNavigationInfoCommand()
{
	delete fNewInfo;
	delete fOldInfo;
}

// InitCheck
status_t
SetNavigationInfoCommand::InitCheck()
{
	if (!fItem)
		return B_NO_INIT;

	if (fNewInfo == NULL && fOldInfo == NULL)
		return B_ERROR;

	if (fNewInfo != NULL && fOldInfo != NULL) {
		if (*fNewInfo == *fOldInfo)
			return B_ERROR;
	}

	return B_OK;
}

// Perform
status_t
SetNavigationInfoCommand::Perform()
{
	fItem->SetNavigationInfo(fNewInfo);
	return B_OK;
}

// Undo
status_t
SetNavigationInfoCommand::Undo()
{
	fItem->SetNavigationInfo(fOldInfo);
	return B_OK;
}

// GetName
void
SetNavigationInfoCommand::GetName(BString& name)
{
	if (fNewInfo == NULL)
		name << "Remove Navigation Info";
	else if (fOldInfo == NULL)
		name << "Add Navigation Info";
	else
		name << "Edit Navigation Info";
}


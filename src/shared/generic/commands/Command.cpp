/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Command.h"

#include <stdio.h>

#include <OS.h>

// constructor
Command::Command()
	: fTimeStamp(system_time())
{
}

// destructor
Command::~Command()
{
}

// InitCheck
status_t
Command::InitCheck()
{
	return B_NO_INIT;
}

// Perform
status_t
Command::Perform()
{
	return B_ERROR;
}

// Undo
status_t
Command::Undo()
{
	return B_ERROR;
}

// Redo
status_t
Command::Redo()
{
	return Perform();
}

// GetName
void
Command::GetName(BString& name)
{
	name << "Name of action goes here.";
}

// UndoesPrevious
bool
Command::UndoesPrevious(const Command* previous) const
{
	return false;
}

// CombineWithNext
bool
Command::CombineWithNext(const Command* next)
{
	return false;
}

// CombineWithPrevious
bool
Command::CombineWithPrevious(const Command* previous)
{
	return false;
}

// #pragma mark -

// _GetString
const char*
Command::_GetString(uint32 key, const char* defaultString) const
{
//	if (LanguageManager* manager = LanguageManager::Default())
//		return manager->GetString(key, defaultString);
//	else
		return defaultString;
}

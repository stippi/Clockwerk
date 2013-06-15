/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ExecuteClip.h"

#include "CommonPropertyIDs.h"
#include "Property.h"

// constructor
ExecuteClip::ExecuteClip(const char* name)
	: Clip("ExecuteClip", name)

	, fCommandPath(dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_COMMAND_PATH)))
{
}

// constructor
ExecuteClip::ExecuteClip(const ExecuteClip& other)
	: Clip(other, true)

	, fCommandPath(dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_COMMAND_PATH)))
{
}

// destructor
ExecuteClip::~ExecuteClip()
{
}

// Duration
uint64
ExecuteClip::Duration()
{
	// TODO: ...
	return 20;
}

// Bounds
BRect
ExecuteClip::Bounds(BRect canvasBounds)
{
	return BRect(0, 0, -1, -1);
}

// #pragma mark -

// CommandPath
const char*
ExecuteClip::CommandPath() const
{
	if (fCommandPath)
		return fCommandPath->Value();
	return "";
}


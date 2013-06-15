/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "NavigationInfo.h"

#include <Message.h>

//#include "XMLHelper.h"

static const char* kTargetIDAttr = "target_id";

// constructor
NavigationInfo::NavigationInfo()
	: fTargetID("")
{
}

// constructor
NavigationInfo::NavigationInfo(const NavigationInfo& other)
	:
	fTargetID(other.fTargetID)
{
}

// constructor
NavigationInfo::NavigationInfo(BMessage* archive)
	:
	fTargetID("")
{
	if (!archive)
		return;

	if (archive->FindString(kTargetIDAttr, &fTargetID) != B_OK)
		fTargetID = "";
}

// destructor
NavigationInfo::~NavigationInfo()
{
}

// #pragma mark -

// operator==
bool
NavigationInfo::operator==(const NavigationInfo& other) const
{
	return fTargetID == other.fTargetID;
}

// Archive
status_t
NavigationInfo::Archive(BMessage* archive) const
{
	if (!archive)
		return B_BAD_VALUE;

	return archive->AddString(kTargetIDAttr, fTargetID);
}

//// XMLStore
//status_t
//NavigationInfo::XMLStore(XMLHelper& xml) const
//{
//	return xml.SetAttribute(kTargetIDAttr, fTargetID.String());
//}
//
//// XMLRestore
//status_t
//NavigationInfo::XMLRestore(XMLHelper& xml)
//{
//	status_t ret = xml.GetAttribute(kTargetIDAttr, fTargetID);
//	if (ret != B_OK)
//		fTargetID = "";
//	return ret;
//}

// #pragma mark -

// SetTargetID
void
NavigationInfo::SetTargetID(const char* id)
{
	fTargetID = id;
}



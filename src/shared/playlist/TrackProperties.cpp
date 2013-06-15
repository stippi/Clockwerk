/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TrackProperties.h"

#include <Message.h>

//#include "XMLHelper.h"

// constructor
TrackProperties::TrackProperties(uint32 track)
	:
	fTrack(track),
	fEnabled(true),
	fName(""),
	fAlpha(255)
{
}

// constructor
TrackProperties::TrackProperties(const BMessage* archive)
	:
	fTrack(0),
	fEnabled(true),
	fName(""),
	fAlpha(255)
{
	Unarchive(archive);
}

// constructor
TrackProperties::TrackProperties(const TrackProperties& other)
	:
	fTrack(other.fTrack),
	fEnabled(other.fEnabled),
	fName(other.fName),
	fAlpha(other.fAlpha)
{
}

// destructor
TrackProperties::~TrackProperties()
{
}

// operator=
TrackProperties&
TrackProperties::operator=(const TrackProperties& other)
{
	fTrack = other.fTrack;
	fEnabled = other.fEnabled;
	fName = other.fName;
	fAlpha = other.fAlpha;

	return *this;
}

// operator==
bool
TrackProperties::operator==(const TrackProperties& other) const
{
	return fTrack == other.fTrack
		&& fEnabled == other.fEnabled && fName == other.fName
		&& fAlpha == other.fAlpha;
}

// operator!=
bool
TrackProperties::operator!=(const TrackProperties& other) const
{
	return !(*this == other);
}

// #pragma mark - XMLStorable

//// XMLStore
//status_t
//TrackProperties::XMLStore(XMLHelper& xml) const
//{
//	status_t ret = xml.SetAttribute("track", fTrack);
//
//	if (ret == B_OK && !fEnabled)
//		ret = xml.SetAttribute("enabled", fEnabled);
//
//	if (ret == B_OK && fName.CountChars() > 0)
//		ret = xml.SetAttribute("name", fName);
//
//	if (ret == B_OK && fAlpha != 255)
//		ret = xml.SetAttribute("alpha", fAlpha);
//
//	return ret;
//}
//
//// XMLRestore
//status_t
//TrackProperties::XMLRestore(XMLHelper& xml)
//{
//	// restore track, error is not acceptable
//	int32 track = xml.GetAttribute("track", (int32)-1);
//	if (track < 0)
//		return B_ERROR;
//	SetTrack((uint32)track);
//
//	// restore enabled
//	SetEnabled(xml.GetAttribute("enabled", true));
//	// restore name
//	SetName(xml.GetAttribute("name", ""));
//	// restore alpha
//	SetAlpha(xml.GetAttribute("alpha", (uint8)255));
//
//	return B_OK;
//}

// Archive
status_t
TrackProperties::Archive(BMessage* into, bool deep) const
{
	if (into == NULL)
		return B_BAD_VALUE;

	status_t ret = into->AddUInt32("track", fTrack);

	if (ret == B_OK && !fEnabled)
		ret = into->AddBool("enabled", fEnabled);

	if (ret == B_OK && fName.CountChars() > 0)
		ret = into->AddString("name", fName);

	if (ret == B_OK && fAlpha != 255)
		ret = into->AddUInt8("alpha", fAlpha);

	return ret;
}

// Unarchive
status_t
TrackProperties::Unarchive(const BMessage* from)
{
	if (from == NULL)
		return B_BAD_VALUE;

	// restore track, error is not acceptable
	uint32 track;
	if (from->FindUInt32("track", &track) < B_OK)
		return B_ERROR;
	SetTrack(track);

	// restore enabled
	bool enabled;
	if (from->FindBool("enabled", &enabled) != B_OK)
		enabled = true;
	SetEnabled(enabled);
	// restore name
	const char* name;
	if (from->FindString("name", &name) != B_OK)
		name = "";
	SetName(name);
	// restore alpha
	uint8 alpha;
	if (from->FindUInt8("alpha", &alpha) != B_OK)
		alpha = 255;
	SetAlpha(alpha);

	return B_OK;
}

// #pragma mark - properties

// SetTrack
void
TrackProperties::SetTrack(uint32 track)
{
	fTrack = track;
}

// SetEnabled
void
TrackProperties::SetEnabled(bool enabled)
{
	fEnabled = enabled;
}

// SetName
void
TrackProperties::SetName(const BString& name)
{
	fName = name;
}

// SetAlpha
void
TrackProperties::SetAlpha(uint8 alpha)
{
	fAlpha = alpha;
}

// #pragma mark - convenience

// IsDefault
bool
TrackProperties::IsDefault() const
{
	TrackProperties temp(fTrack);
	return *this == temp;
}



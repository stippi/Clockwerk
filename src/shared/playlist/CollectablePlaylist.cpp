/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "CollectablePlaylist.h"

#include "support_date.h"

#include "CommonPropertyIDs.h"
#include "Property.h"


// constructor 
CollectablePlaylist::CollectablePlaylist()
	: StretchingPlaylist("CollectablePlaylist")
{
}

// constructor 
CollectablePlaylist::CollectablePlaylist(const CollectablePlaylist& other)
	: StretchingPlaylist(other)
{
}

// destructor
CollectablePlaylist::~CollectablePlaylist()
{
}

// #pragma mark -

// SetTypeMarker
void
CollectablePlaylist::SetTypeMarker(const char* typeMarker)
{
	SetValue(PROPERTY_TYPE_MARKER, typeMarker);
}

// TypeMarker
const char*
CollectablePlaylist::TypeMarker() const
{
	return Value(PROPERTY_TYPE_MARKER, (const char*)NULL);
}

// SetSequenceIndex
void
CollectablePlaylist::SetSequenceIndex(int32 index)
{
	SetValue(PROPERTY_SEQUENCE_INDEX, index);
}

// SequenceIndex
int32
CollectablePlaylist::SequenceIndex() const
{
	return Value(PROPERTY_SEQUENCE_INDEX, (int32)0);
}

// #pragma mark -

// SetStartDate
void
CollectablePlaylist::SetStartDate(const char* date)
{
	SetValue(PROPERTY_START_DATE, date);
}

// StartDate
const char*
CollectablePlaylist::StartDate() const
{
	return Value(PROPERTY_START_DATE, (const char*)NULL);
}

// SetValidDayCount
void
CollectablePlaylist::SetValidDayCount(int32 days)
{
	SetValue(PROPERTY_VALID_DAYS, days);
}

// ValidDays
int32
CollectablePlaylist::ValidDays() const
{
	return Value(PROPERTY_VALID_DAYS, (int32)1);
}

// IsValidToday
bool
CollectablePlaylist::IsValidToday() const
{
	return today_is_within_range(StartDate(), ValidDays());
}

// PreferredDuration
uint64
CollectablePlaylist::PreferredDuration() const
{
	return Value(PROPERTY_DURATION, (int64)(9 * 25));
}

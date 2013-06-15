/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleObjectListView.h"

#include <parsedate.h>
#include <stdio.h>

#include <Message.h>
#include <Messenger.h>

#include "support_date.h"

#include "CommonPropertyIDs.h"
#include "Property.h"
#include "Schedule.h"

enum {
	MSG_SCHEDULE_PROPERTIES_CHANGED	= 'scpc',
};

// constructor
ScheduleObjectListView::ScheduleObjectListView(const char* name,
		BMessage* message, BHandler* target)
	: ObjectListView(name, message, target)
	, fYear(0)
	, fWeek(-1)
	, fScope("all")
{
}

// destructor
ScheduleObjectListView::~ScheduleObjectListView()
{
}

// InitiateDrag
bool
ScheduleObjectListView::InitiateDrag(BPoint point, int32 index,
	bool wasSelected)
{
	// no drag&drop for this listview...
	return false;
}

// MessageReceived
void
ScheduleObjectListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SCHEDULE_PROPERTIES_CHANGED:
			Sync();
			break;
		default:
			ObjectListView::MessageReceived(message);
			break;
	}
}

// ObjectAdded
void
ScheduleObjectListView::ObjectAdded(ServerObject* object, int32 index)
{
	if (Schedule* schedule = dynamic_cast<Schedule*>(object)) {
		schedule->Acquire();
		schedule->AddObserver(this);
	}
	ObjectListView::ObjectAdded(object, index);
}

// ObjectRemoved
void
ScheduleObjectListView::ObjectRemoved(ServerObject* object)
{
	if (Schedule* schedule = dynamic_cast<Schedule*>(object)) {
		schedule->RemoveObserver(this);
		schedule->Release();
	}
	ObjectListView::ObjectRemoved(object);
}

// AcceptObject
bool
ScheduleObjectListView::AcceptObject(ServerObject* object)
{
	Schedule* schedule = dynamic_cast<Schedule*>(object);
	if (schedule != NULL) {
		return _AcceptSchedule(schedule);
	}
	return false;
}

// ObjectChanged
void
ScheduleObjectListView::ObjectChanged(const Observable* object)
{
	BMessenger messenger(this);
	messenger.SendMessage(MSG_SCHEDULE_PROPERTIES_CHANGED, this);
}

// #pragma mark -

// SetYear
void
ScheduleObjectListView::SetYear(int32 year)
{
	fYear = year;
	Sync();
}

// SetWeek
void
ScheduleObjectListView::SetWeek(int32 week)
{
	fWeek = week;
	Sync();
}

// SetScope
void
ScheduleObjectListView::SetScope(const char* scope)
{
	fScope = scope;
	Sync();
}


// #pragma mark -

// _AcceptSchedule
bool
ScheduleObjectListView::_AcceptSchedule(Schedule* schedule)
{
	if (fScope != "all" && fScope != "") {
		// filter by scope
		BString scope;
		schedule->GetValue(PROPERTY_SCOPE, scope);
		if (fScope != scope)
			return false;
	}
	if (fWeek > -1 && fYear > 0) {
		// filter by week
		BString date;
		schedule->GetValue(PROPERTY_DATE, date);

		time_t nowSeconds = time(NULL);
		time_t validFromSeconds = parsedate(date.String(), nowSeconds);

		time_t minValidSeconds = unix_time_for_week(fWeek, fYear);
		time_t maxValidSeconds = unix_time_for_week(fWeek + 1, fYear);

		tm valid = *localtime(&validFromSeconds);
		tm max = *localtime(&maxValidSeconds);

		return validFromSeconds >= minValidSeconds
			&& validFromSeconds < maxValidSeconds;
	}

	return true;
}


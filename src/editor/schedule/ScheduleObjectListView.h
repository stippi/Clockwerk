/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_OBJECT_LIST_VIEW_H
#define SCHEDULE_OBJECT_LIST_VIEW_H


#include <String.h>

#include "ObjectListView.h"
#include "Observer.h"


class Schedule;

class ScheduleObjectListView : public ObjectListView,
							   public Observer {
 public:
								ScheduleObjectListView(const char* name,
									BMessage* selectionChangeMessage = NULL,
									BHandler* target = NULL);
	virtual						~ScheduleObjectListView();

	// DragSortableListView interface
	virtual	bool			InitiateDrag(BPoint point, int32 index,
										 bool wasSelected);
	virtual	void				MessageReceived(BMessage* message);

	// ObjectListView interface
	virtual	bool				AcceptObject(ServerObject* object);

 protected:
	virtual	void				ObjectAdded(ServerObject* object,
											int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

 public:
	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);


	// ScheduleObjectListView
			void				SetYear(int32 year);
			void				SetWeek(int32 week);
			void				SetScope(const char* scope);

 private:
			bool				_AcceptSchedule(Schedule* schedule);

			int32				fYear;
			int32				fWeek;
			BString				fScope;
};

#endif // SCHEDULE_OBJECT_LIST_VIEW_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_OBSERVER_H
#define SCHEDULE_OBSERVER_H

#include <SupportDefs.h>

class ScheduleItem;

class ScheduleObserver {
 public:
								ScheduleObserver();
	virtual						~ScheduleObserver();

	virtual	void				ItemAdded(ScheduleItem* item, int32 index);
	virtual	void				ItemRemoved(ScheduleItem* item);

	virtual	void				ItemsLayouted();

	virtual	void				NotificationBlockStarted();
	virtual	void				NotificationBlockFinished();
};

#endif // SCHEDULE_OBSERVER_H

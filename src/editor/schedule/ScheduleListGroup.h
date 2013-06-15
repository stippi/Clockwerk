/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_LIST_GROUP_H
#define SCHEDULE_LIST_GROUP_H


#include <String.h>
#include <View.h>

#include "ScheduleObjectListView.h"


class BMenu;
class BMenuField;
class BTextControl;
class ScopeMenuField;

class ScheduleListGroup : public BView {
 public:
								ScheduleListGroup(
									ScheduleObjectListView* listView);
	virtual						~ScheduleListGroup();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				AllAttached();
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				MessageReceived(BMessage* message);

	// ScheduleListGroup
			ScheduleObjectListView* ListView() const
									{ return fScheduleListView; }

			// of which week schedules in the listview shoud show:
			void				SetWeek(int32 week);
			int32				Week() const
									{ return fWeek; }

			void				SetYear(int32 year);
			int32				Year() const
									{ return fYear; }

			void				SetScope(const char* scope);
			const char*			Scope() const
									{ return fScope.String(); }

			void				SetScopes(const BMessage* scopes);

 private:
			BMenu*				_CreateMonthMenu(int32 monthIndex,
									const char* monthName) const;
			void				_BuildWeekMenu(BMenu* menu) const;

			ScheduleObjectListView* fScheduleListView;
			BMenuField*			fWeekMF;
			BMenu*				fWeekM;
			BTextControl*		fYearTC;
			ScopeMenuField*		fScopeMF;

			BRect				fPreviousBounds;

			int32				fWeek;
			int32				fYear;
			BString				fScope;
};

#endif // SCHEDULE_LIST_GROUP_H

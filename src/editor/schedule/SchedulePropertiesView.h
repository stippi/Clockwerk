/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_PROPERTIES_VIEW_H
#define SCHEDULE_PROPERTIES_VIEW_H

#include <View.h>

#include "Observer.h"

class BBox;
class BCheckBox;
class BControl;
class BMenuField;
class BTextControl;
class CommandStack;
class ScopeMenuField;
class Schedule;
class ScheduleItem;
class Selection;

class SchedulePropertiesView : public BView,
							   public Observer {
 public:
								SchedulePropertiesView();
	virtual						~SchedulePropertiesView();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);

	// Observer interface
	virtual void				ObjectChanged(const Observable* object);

	// SchedulePropertiesView
			void				SetSchedule(Schedule* schedule);
			void				SetCommandStack(CommandStack* stack);
			void				SetSelection(Selection* selection);
			void				SetScopes(const BMessage* scopes);

 private:
			void				_ObjectChanged(const Observable* object);

			void				_SetSelectedScheduleItem(ScheduleItem* item);
			void				_AddControl(BView* control, BView* parent,
									BRect bounds, BRect& frame);
			void				_Relayout();
			void				_AdoptScheduleProperties();
			void				_AdoptScheduleScope();
			void				_AdoptScheduleType();
			void				_MarkStatusItem(int32 status);
			void				_MarkTypeItem(uint32 type);
			void				_AdoptScheduleItemProperties();

			template<class Control>
			void				_EnableControl(Control* control, bool enable);

			Schedule*			fSchedule;
			CommandStack*		fCommandStack;
			Selection*			fSelection;

			ScheduleItem*		fSelectedScheduleItem;

			// Schedule controls
			BTextControl*		fNameTC;
			ScopeMenuField*		fScopeMF;
			BMenuField*			fStatusMF;
			BMenuField*			fTypeMF;
			BTextControl*		fDateTC;

			BBox*				fWeekDaysGroup;
			BCheckBox*			fMondayCB;
			BCheckBox*			fTuesdayCB;
			BCheckBox*			fWednesdayCB;
			BCheckBox*			fThursdayCB;
			BCheckBox*			fFridayCB;
			BCheckBox*			fSaturdayCB;
			BCheckBox*			fSundayCB;

			// ScheduleItem controls
			BTextControl*		fStartTimeTC;
			BTextControl*		fDurationTC;
			BTextControl*		fRepeatsTC;
			BCheckBox*			fFixedStartFrameCB;
			BCheckBox*			fFlexibleDurationCB;
};

#endif // SCHEDULE_PROPERTIES_VIEW_H

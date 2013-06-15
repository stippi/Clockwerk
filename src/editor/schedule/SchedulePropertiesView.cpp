/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SchedulePropertiesView.h"

#include <Box.h>
#include <CheckBox.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <Looper.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include "support.h"
#include "support_date.h"

#include "ChangeScheduleItemCommand.h"
#include "CommonPropertyIDs.h"
#include "CommandStack.h"
#include "OptionProperty.h"
#include "Playlist.h"
#include "PropertyCommand.h"
#include "ScopeMenuField.h"
#include "Schedule.h"
#include "ScheduleItem.h"
#include "Selection.h"
#include "Separator.h"
#include "WeekDaysProperty.h"

enum {
	// Schedule controls messages
	MSG_SET_NAME					= 'stnm',
	MSG_SET_STATUS					= 'stst',
	MSG_SET_TYPE					= 'sttp',
	MSG_SET_DATE					= 'stdt',
	MSG_TOGGLE_DAY					= 'tgld',

	// ScheduleItem controls messages
	MSG_SET_START_FRAME				= 'stsf',
	MSG_SET_DURATION				= 'stdr',
	MSG_SET_REPEAT_COUNT			= 'strc',
	MSG_TOGGLE_FLEXIBLE_STARTFRAME	= 'tgfs',
	MSG_TOGGLE_FLEXIBLE_DURATION	= 'tgfd',
	MSG_TOGGLE_ALLOW_REPEATS		= 'tgar',

	MSG_OBJECT_CHANGED				= 'objc',
};

static const int64 kWholeDayDuration = (24 * 60 * 60) * 25;

// constructor
SchedulePropertiesView::SchedulePropertiesView()
	: BView(BRect(0, 0, 199, 199), "schedule properties view",
		B_FOLLOW_NONE, B_WILL_DRAW | B_FRAME_EVENTS)
	, fSchedule(NULL)
	, fCommandStack(NULL)
	, fSelection(NULL)

	, fSelectedScheduleItem(NULL)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BRect bounds(Bounds());

	BRect frame(bounds);
	frame.InsetBy(5, 5);
	frame.bottom = frame.top + 15;
	BSeparatorView* separator = new BSeparatorView("Schedule Properties");
	_AddControl(separator, this, bounds, frame);

	fNameTC = new BTextControl(frame, "name tc", "Name", "",
		new BMessage(MSG_SET_NAME));
	_AddControl(fNameTC, this, bounds, frame);

	// scope menu field
	BMenu* scopeMenu = new BPopUpMenu("<unset>");
	fScopeMF = new ScopeMenuField(frame, "scope mf", "Scope", scopeMenu, true);
	float width, height;
	fScopeMF->MenuBar()->GetPreferredSize(&width, &height);
	fScopeMF->MenuBar()->ResizeTo(width, height);
	fScopeMF->MenuBar()->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fScopeMF->ResizeTo(frame.Width(), height + 6);
	frame.bottom = frame.top + height + 6;

	_AddControl(fScopeMF, this, bounds, frame);

	BMessage* message;
	BMenuItem* menuItem;

	// status menu field
	BMenu* statusMenu = new BPopUpMenu("<select>");
	message = new BMessage(MSG_SET_STATUS);
	message->AddInt32("status", PLAYLIST_STATUS_DRAFT);
	menuItem = new BMenuItem("Draft", message);
	statusMenu->AddItem(menuItem);
	menuItem->SetMarked(true);
	message = new BMessage(MSG_SET_STATUS);
	message->AddInt32("status", PLAYLIST_STATUS_TESTING);
	statusMenu->AddItem(new BMenuItem("Testing", message));
	message = new BMessage(MSG_SET_STATUS);
	message->AddInt32("status", PLAYLIST_STATUS_READY);
	statusMenu->AddItem(new BMenuItem("Ready", message));
	message = new BMessage(MSG_SET_STATUS);
	message->AddInt32("status", PLAYLIST_STATUS_LIVE);
	statusMenu->AddItem(new BMenuItem("Live", message));

	fStatusMF = new BMenuField(frame, "scope mf", "Status", statusMenu, true);
	fStatusMF->MenuBar()->GetPreferredSize(&width, &height);
	fStatusMF->MenuBar()->ResizeTo(width, height);
	fStatusMF->MenuBar()->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fStatusMF->ResizeTo(frame.Width(), height + 6);
	frame.bottom = frame.top + height + 6;

	_AddControl(fStatusMF, this, bounds, frame);

	// type menuField
	BMenu* typeMenu = new BPopUpMenu("type pu");
	message = new BMessage(MSG_SET_TYPE);
	message->AddInt32("type", SCHEDULE_TYPE_WEEKLY);
	menuItem = new BMenuItem("Weekly", message);
	typeMenu->AddItem(menuItem);
	menuItem->SetMarked(true);
	message = new BMessage(MSG_SET_TYPE);
	message->AddInt32("type", SCHEDULE_TYPE_DATE);
	typeMenu->AddItem(new BMenuItem("Specific Date", message));

	fTypeMF = new BMenuField(frame, "type mf", "Type", typeMenu, true);
	fTypeMF->MenuBar()->GetPreferredSize(&width, &height);
	fTypeMF->MenuBar()->ResizeTo(width, height);
	fTypeMF->MenuBar()->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fTypeMF->ResizeTo(frame.Width(), height + 6);
	frame.bottom = frame.top + height + 6;

	_AddControl(fTypeMF, this, bounds, frame);

	frame.bottom = frame.top + 15;
	frame.right = bounds.right - 5;
	fDateTC = new BTextControl(frame, "date tc", "Date", "",
		new BMessage(MSG_SET_DATE));
	_AddControl(fDateTC, this, bounds, frame);

	BRect groupBounds(bounds);
	groupBounds.top = frame.bottom + 8;
	groupBounds.left += 5;
	groupBounds.right -= 5;
	groupBounds.bottom -= 5;
	fWeekDaysGroup = new BBox(groupBounds, "week days group");
	fWeekDaysGroup->SetLabel("Week Days");
	_AddControl(fWeekDaysGroup, this, bounds, frame);
	groupBounds.OffsetTo(B_ORIGIN);
	frame = groupBounds;
	frame.top += 15;
	frame.left += 5;
	frame.right -= 5;
	frame.bottom = frame.top + 15;

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::MONDAY);
	fMondayCB = new BCheckBox(frame, "mon cb", "Monday", message);
	_AddControl(fMondayCB, fWeekDaysGroup, groupBounds, frame);

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::TUESDAY);
	fTuesdayCB = new BCheckBox(frame, "tue cb", "Tuesday", message);
	_AddControl(fTuesdayCB, fWeekDaysGroup, groupBounds, frame);

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::WEDNESDAY);
	fWednesdayCB = new BCheckBox(frame, "we cb", "Wednesday", message);
	_AddControl(fWednesdayCB, fWeekDaysGroup, groupBounds, frame);

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::THURSDAY);
	fThursdayCB = new BCheckBox(frame, "thu cb", "Thursday", message);
	_AddControl(fThursdayCB, fWeekDaysGroup, groupBounds, frame);

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::FRIDAY);
	fFridayCB = new BCheckBox(frame, "fri cb", "Friday", message);
	_AddControl(fFridayCB, fWeekDaysGroup, groupBounds, frame);

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::SATURDAY);
	fSaturdayCB = new BCheckBox(frame, "sat cb", "Saturday", message);
	_AddControl(fSaturdayCB, fWeekDaysGroup, groupBounds, frame);

	message = new BMessage(MSG_TOGGLE_DAY);
	message->AddInt32("day", WeekDaysProperty::SUNDAY);
	fSundayCB = new BCheckBox(frame, "sun cb", "Sunday", message);
	_AddControl(fSundayCB, fWeekDaysGroup, groupBounds, frame);

	// resize grouping box to enclose all check boxen
	fWeekDaysGroup->ResizeTo(fWeekDaysGroup->Frame().Width(),
		fSundayCB->Frame().bottom + 5);

	// --- item properties ---

	frame = fWeekDaysGroup->Frame();
	frame.top = frame.bottom + 5;
	frame.bottom = frame.top + 15;
	separator = new BSeparatorView("Item Properties");
	_AddControl(separator, this, bounds, frame);

	fStartTimeTC = new BTextControl(frame, "start time tc", "Starttime", "",
		new BMessage(MSG_SET_START_FRAME));
	_AddControl(fStartTimeTC, this, bounds, frame);

	fDurationTC = new BTextControl(frame, "duration tc", "Duration", "",
		new BMessage(MSG_SET_DURATION));
	_AddControl(fDurationTC, this, bounds, frame);

	fRepeatsTC = new BTextControl(frame, "repeat tc", "Repeats", "",
		new BMessage(MSG_SET_REPEAT_COUNT));
	_AddControl(fRepeatsTC, this, bounds, frame);

	fFixedStartFrameCB = new BCheckBox(frame, "fixed starttime cb",
		"Fixed Starttime", new BMessage(MSG_TOGGLE_FLEXIBLE_STARTFRAME));
	_AddControl(fFixedStartFrameCB, this, bounds, frame);

	fFlexibleDurationCB = new BCheckBox(frame, "flexible duration cb",
		"Flexible Duration", new BMessage(MSG_TOGGLE_FLEXIBLE_DURATION));
	_AddControl(fFlexibleDurationCB, this, bounds, frame);

	// last minute beautification
	float divider = max_c(fNameTC->StringWidth(fNameTC->Label()),
		fTypeMF->StringWidth(fTypeMF->Label()));
	divider = max_c(divider, fScopeMF->StringWidth(fScopeMF->Label()));
	divider = max_c(divider, fStatusMF->StringWidth(fStatusMF->Label()));
	divider = max_c(divider, fDateTC->StringWidth(fDateTC->Label()));
	divider = max_c(divider, fDateTC->StringWidth("Valid From"));

	divider = max_c(divider, fStartTimeTC->StringWidth(fStartTimeTC->Label()));
	divider = max_c(divider, fDurationTC->StringWidth(fDurationTC->Label()));
	divider = max_c(divider, fRepeatsTC->StringWidth(fRepeatsTC->Label()));

	fNameTC->SetDivider(divider + 4);
	fScopeMF->SetDivider(divider + 8);
	fStatusMF->SetDivider(divider + 8);
	fTypeMF->SetDivider(divider + 8);
	fDateTC->SetDivider(divider + 4);

	fScopeMF->MenuBar()->GetPreferredSize(&width, &height);
	fScopeMF->MenuBar()->ResizeTo(width, height);
	fScopeMF->ResizeTo(frame.Width(), height + 6);

	fStatusMF->MenuBar()->GetPreferredSize(&width, &height);
	fStatusMF->MenuBar()->ResizeTo(width, height);
	fStatusMF->ResizeTo(frame.Width(), height + 6);

	fTypeMF->MenuBar()->GetPreferredSize(&width, &height);
	fTypeMF->MenuBar()->ResizeTo(width, height);
	fTypeMF->ResizeTo(frame.Width(), height + 6);

	fStartTimeTC->SetDivider(divider + 4);
	fDurationTC->SetDivider(divider + 4);
	fRepeatsTC->SetDivider(divider + 4);

	// give the controls initial values
	_AdoptScheduleProperties();
	_AdoptScheduleItemProperties();
}

// destructor
SchedulePropertiesView::~SchedulePropertiesView()
{
	SetSchedule(NULL);
	SetCommandStack(NULL);
	_SetSelectedScheduleItem(NULL);
	SetSelection(NULL);
}

// AttachedToWindow
void
SchedulePropertiesView::AttachedToWindow()
{
	fNameTC->SetTarget(this);
	fStatusMF->Menu()->SetTargetForItems(this);
	fScopeMF->Menu()->SetTargetForItems(this);
	fTypeMF->Menu()->SetTargetForItems(this);

	fMondayCB->SetTarget(this);
	fTuesdayCB->SetTarget(this);
	fWednesdayCB->SetTarget(this);
	fThursdayCB->SetTarget(this);
	fFridayCB->SetTarget(this);
	fSaturdayCB->SetTarget(this);
	fSundayCB->SetTarget(this);

	fDateTC->SetTarget(this);

	fStartTimeTC->SetTarget(this);
	fDurationTC->SetTarget(this);
	fRepeatsTC->SetTarget(this);
	fFixedStartFrameCB->SetTarget(this);
	fFlexibleDurationCB->SetTarget(this);
}

// MessageReceived
void
SchedulePropertiesView::MessageReceived(BMessage* message)
{
	if (!fSchedule) {
		BView::MessageReceived(message);
		return;
	}

	// TODO: write lock document lock

	switch (message->what) {
		case MSG_SET_NAME: {
			StringProperty* name = dynamic_cast<StringProperty*>(
				fSchedule->FindProperty(PROPERTY_NAME));

			PropertyCommand::ChangeProperty(fCommandStack,
				fSchedule, name, fNameTC->Text());
			break;
		}
		case MSG_SET_STATUS: {
			int32 statusOption;
			if (message->FindInt32("status", &statusOption) == B_OK) {
				OptionProperty* status = dynamic_cast<OptionProperty*>(
					fSchedule->FindProperty(PROPERTY_STATUS));

				PropertyCommand::ChangeProperty(fCommandStack,
					fSchedule, status, statusOption);
			}
			break;
		}
		case MSG_SET_SCOPE: {
			const char* scope;
			if (message->FindString("scop", &scope) == B_OK) {
				PropertyCommand::ChangeProperty(fCommandStack,
					fSchedule, fSchedule->FindProperty(PROPERTY_SCOPE),
					scope);
			}
			break;
		}
		case MSG_SET_TYPE: {
			uint32 type;
			if (message->FindInt32("type", (int32*)&type) == B_OK) {
				PropertyCommand::ChangeProperty(fCommandStack,
					fSchedule, fSchedule->Type(), type);
				_AdoptScheduleType();
			}
			break;
		}
		case MSG_TOGGLE_DAY: {
			int32 value;
			uint32 day;
			if (message->FindInt32("be:value", &value) == B_OK
				&& message->FindInt32("day", (int32*)&day) == B_OK) {
				uint8 days = fSchedule->WeekDays()->Value();
				if (value)
					days = days | day;
				else
					days = days & ~day;
				PropertyCommand::ChangeProperty(fCommandStack,
					fSchedule, fSchedule->WeekDays(), days);
			}
			break;
		}
		case MSG_SET_DATE: {
			PropertyCommand::ChangeProperty(fCommandStack,
					fSchedule, fSchedule->Date(), fDateTC->Text());
			break;
		}
		// ScheduleItem manipulation
		case MSG_SET_START_FRAME:
		case MSG_SET_DURATION:
			if (fSelectedScheduleItem) {
				const char* string
					= message->what == MSG_SET_DURATION ?
						fDurationTC->Text() : fStartTimeTC->Text();

				int64 frameCount = day_time_to_frame(string);
				if (frameCount >= 0) {
					uint64 startFrame = fSelectedScheduleItem->StartFrame();
					uint64 duration = fSelectedScheduleItem->Duration();
					if (message->what == MSG_SET_START_FRAME) {
						if (frameCount >= 0 && frameCount < kWholeDayDuration)
							startFrame = (uint64)frameCount;
						// NOTE: we don't filter, exactly because that
						// would never change the start time -> the text
						// control always enforces
					} else {
						if (frameCount > 0 && frameCount < kWholeDayDuration) {
							duration = (uint64)frameCount;
							fSelectedScheduleItem->FilterDuration(&duration);
						}
					}
					fCommandStack->Perform(
						new (std::nothrow) ChangeScheduleItemCommand(
							fSchedule, fSelection, fSelectedScheduleItem,
							startFrame, duration,
							fSelectedScheduleItem->ExplicitRepeats(),
							fSelectedScheduleItem->FlexibleStartFrame(),
							fSelectedScheduleItem->FlexibleDuration()));
					_AdoptScheduleItemProperties();
				}
			}
			break;
		case MSG_SET_REPEAT_COUNT:
			if (fSelectedScheduleItem) {
				const char* string = fRepeatsTC->Text();
				int32 repeats;
				if (sscanf(string, "%ld", &repeats) == 1
					&& repeats >= 0 && repeats < 100000) {
					Playlist* playlist = fSelectedScheduleItem->Playlist();
					if (playlist && playlist->Duration() > 0) {
						fCommandStack->Perform(
							new (std::nothrow) ChangeScheduleItemCommand(
								fSchedule, fSelection, fSelectedScheduleItem,
								fSelectedScheduleItem->StartFrame(),
								fSelectedScheduleItem->Duration(),
								repeats,
								fSelectedScheduleItem->FlexibleStartFrame(),
								fSelectedScheduleItem->FlexibleDuration()));
					}
				}
			}
			break;
		case MSG_TOGGLE_FLEXIBLE_STARTFRAME:
			if (fSelectedScheduleItem) {
				fCommandStack->Perform(
					new (std::nothrow) ChangeScheduleItemCommand(
						fSchedule, fSelection, fSelectedScheduleItem,
						fSelectedScheduleItem->StartFrame(),
						fSelectedScheduleItem->Duration(),
						fSelectedScheduleItem->ExplicitRepeats(),
						fFixedStartFrameCB->Value() != B_CONTROL_ON,
						fSelectedScheduleItem->FlexibleDuration()));
			}
			break;
		case MSG_TOGGLE_FLEXIBLE_DURATION:
			if (fSelectedScheduleItem) {
				fCommandStack->Perform(
					new (std::nothrow) ChangeScheduleItemCommand(
						fSchedule, fSelection, fSelectedScheduleItem,
						fSelectedScheduleItem->StartFrame(),
						fSelectedScheduleItem->Duration(),
						fSelectedScheduleItem->ExplicitRepeats(),
						fSelectedScheduleItem->FlexibleStartFrame(),
						fFlexibleDurationCB->Value() == B_CONTROL_ON));
			}
			break;

		case MSG_OBJECT_CHANGED: {
			const Observable* object;
			if (message->FindPointer("object", (void**)&object) == B_OK)
				_ObjectChanged(object);
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}

// FrameResized
void
SchedulePropertiesView::FrameResized(float width, float height)
{
	_Relayout();
}

// Draw
void
SchedulePropertiesView::Draw(BRect updateRect)
{
	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lighten2 = tint_color(background, B_LIGHTEN_2_TINT);
	rgb_color darken2 = tint_color(background, B_DARKEN_2_TINT);

	BRect b(Bounds());

	BeginLineArray(3);
		AddLine(BPoint(b.left, b.bottom),
				BPoint(b.left, b.top), lighten2);
		AddLine(BPoint(b.left + 1, b.top),
				BPoint(b.right, b.top), lighten2);
		AddLine(BPoint(b.right, b.top + 1),
				BPoint(b.right, b.bottom), darken2);
	EndLineArray();
}

// #pragma mark -

// ObjectChanged
void
SchedulePropertiesView::ObjectChanged(const Observable* object)
{
	if (!Looper()) {
		_ObjectChanged(object);
		return;
	}

	BMessage message(MSG_OBJECT_CHANGED);
	message.AddPointer("object", object);

	Looper()->PostMessage(&message, this);
}

// #pragma mark -

// SetSchedule
void
SchedulePropertiesView::SetSchedule(Schedule* schedule)
{
	if (fSchedule == schedule)
		return;

	if (fSchedule) {
		fSchedule->RemoveObserver(this);
		fSchedule->Release();
	}

	fSchedule = schedule;

	if (fSchedule) {
		fSchedule->Acquire();
		fSchedule->AddObserver(this);
	}

	_SetSelectedScheduleItem(NULL);
	_AdoptScheduleProperties();
}

// SetCommandStack
void
SchedulePropertiesView::SetCommandStack(CommandStack* stack)
{
	fCommandStack = stack;
}

// SetSelection
void
SchedulePropertiesView::SetSelection(Selection* selection)
{
	if (fSelection == selection)
		return;

	if (fSelection)
		fSelection->RemoveObserver(this);

	fSelection = selection;

	if (fSelection)
		fSelection->AddObserver(this);
}

// SetScopes
void
SchedulePropertiesView::SetScopes(const BMessage* scopes)
{
	fScopeMF->SetScopes(scopes);
}

// #pragma mark -

// _ObjectChanged
void
SchedulePropertiesView::_ObjectChanged(const Observable* object)
{
	if (object == fSchedule) {
		_AdoptScheduleProperties();
	} else if (object == fSelection) {
		// set to first selected ScheduleItem, unless the schedules
		// don't match
		Selectable* selectable = fSelection->SelectableAt(0);
		ScheduleItem* item = dynamic_cast<ScheduleItem*>(selectable);
		if (item && item->Parent() != fSchedule)
			item = NULL;
		_SetSelectedScheduleItem(item);
	} else if (object == fSelectedScheduleItem) {
		_AdoptScheduleItemProperties();
	}
}

// _SetSelectedScheduleItem
void
SchedulePropertiesView::_SetSelectedScheduleItem(ScheduleItem* item)
{
	if (fSelectedScheduleItem == item)
		return;

	if (fSelectedScheduleItem) {
		fSelectedScheduleItem->RemoveObserver(this);
		fSelectedScheduleItem->Release();
	}

	fSelectedScheduleItem = item;

	if (fSelectedScheduleItem) {
		fSelectedScheduleItem->Acquire();
		fSelectedScheduleItem->AddObserver(this);
	}

	_AdoptScheduleItemProperties();
}

// _AddControl
void
SchedulePropertiesView::_AddControl(BView* control, BView* parent,
	BRect bounds, BRect& frame)
{
	float width, height;
	control->GetPreferredSize(&width, &height);
	frame.bottom = frame.top + height;
	frame.left = bounds.left + 5;
	frame.right = max_c(bounds.right - 5, frame.left + width);
	control->MoveTo(frame.left, frame.top);
	control->ResizeTo(frame.Width(), frame.Height());
	parent->AddChild(control);
	control->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	// layout frame for next control
	frame.top = frame.bottom + 5;
	frame.bottom = frame.top + 15;
}

// _Relayout
void
SchedulePropertiesView::_Relayout()
{
}

// _AdoptScheduleProperties
void
SchedulePropertiesView::_AdoptScheduleProperties()
{
	bool enable = fSchedule != NULL;
	_EnableControl(fNameTC, enable);
	_EnableControl(fScopeMF, enable);
	_EnableControl(fStatusMF, enable);
	_EnableControl(fTypeMF, enable);

	_EnableControl(fMondayCB, enable);
	_EnableControl(fTuesdayCB, enable);
	_EnableControl(fWednesdayCB, enable);
	_EnableControl(fThursdayCB, enable);
	_EnableControl(fFridayCB, enable);
	_EnableControl(fSaturdayCB, enable);
	_EnableControl(fSundayCB, enable);

	_EnableControl(fDateTC, enable);

	if (fSchedule) {
		fNameTC->SetText(fSchedule->Name().String());
		_MarkTypeItem(fSchedule->Type()->CurrentOptionID());
		OptionProperty* status = dynamic_cast<OptionProperty*>(
			fSchedule->FindProperty(PROPERTY_STATUS));
		if (status)
			_MarkStatusItem(status->CurrentOptionID());
		_AdoptScheduleScope();

		uint8 weekDays = fSchedule->WeekDays()->Value();
		fMondayCB->SetValue(weekDays & WeekDaysProperty::MONDAY);
		fTuesdayCB->SetValue(weekDays & WeekDaysProperty::TUESDAY);
		fWednesdayCB->SetValue(weekDays & WeekDaysProperty::WEDNESDAY);
		fThursdayCB->SetValue(weekDays & WeekDaysProperty::THURSDAY);
		fFridayCB->SetValue(weekDays & WeekDaysProperty::FRIDAY);
		fSaturdayCB->SetValue(weekDays & WeekDaysProperty::SATURDAY);
		fSundayCB->SetValue(weekDays & WeekDaysProperty::SUNDAY);

		fDateTC->SetText(fSchedule->Date()->Value());
	} else {
		fNameTC->SetText("<select a schedule>");

		fMondayCB->SetValue(false);
		fTuesdayCB->SetValue(false);
		fWednesdayCB->SetValue(false);
		fThursdayCB->SetValue(false);
		fFridayCB->SetValue(false);
		fSaturdayCB->SetValue(false);
		fSundayCB->SetValue(false);

		fDateTC->SetText("");
	}

	_AdoptScheduleType();
}

// _AdoptScheduleScope
void
SchedulePropertiesView::_AdoptScheduleScope()
{
	if (!fSchedule)
		return;

	BString scope;
	fSchedule->GetValue(PROPERTY_SCOPE, scope);

	fScopeMF->MarkScope(scope.String());
}

// _AdoptScheduleType
void
SchedulePropertiesView::_AdoptScheduleType()
{
	if (!fSchedule)
		return;

	// hide/show weekdays group or date text control depending
	// on schedule type
	if (fSchedule->Type()->CurrentOptionID() == SCHEDULE_TYPE_WEEKLY) {
		fDateTC->SetLabel("Valid From");
		if (fWeekDaysGroup->IsHidden())
			fWeekDaysGroup->Show();
	} else {
		fDateTC->SetLabel("Date");
		if (!fWeekDaysGroup->IsHidden())
			fWeekDaysGroup->Hide();
	}
}

// Bah... code duplication...
// _MarkStatusItem
void
SchedulePropertiesView::_MarkStatusItem(int32 status)
{
	for (int32 i = 0; BMenuItem* item = fStatusMF->Menu()->ItemAt(i); i++) {
		BMessage* message = item->Message();
		if (!message)
			continue;

		int32 itemStatus;
		if (message->FindInt32("status", &itemStatus) < B_OK)
			continue;

		if (itemStatus == status) {
			item->SetMarked(true);
			break;
		}
	}
}

// _MarkTypeItem
void
SchedulePropertiesView::_MarkTypeItem(uint32 type)
{
	for (int32 i = 0; BMenuItem* item = fTypeMF->Menu()->ItemAt(i); i++) {
		BMessage* message = item->Message();
		if (!message)
			continue;

		uint32 itemType;
		if (message->FindInt32("type", (int32*)&itemType) < B_OK)
			continue;

		if (itemType == type) {
			item->SetMarked(true);
			break;
		}
	}
}

// _AdoptScheduleItemProperties
void
SchedulePropertiesView::_AdoptScheduleItemProperties()
{
	bool enable = fSelectedScheduleItem != NULL;
	_EnableControl(fStartTimeTC, enable);
	_EnableControl(fDurationTC, enable);
	_EnableControl(fFixedStartFrameCB, enable);
	_EnableControl(fFlexibleDurationCB, enable);

	if (fSelectedScheduleItem) {
		BString helper;
		string_for_frame_of_day(helper, fSelectedScheduleItem->StartFrame());
		fStartTimeTC->SetText(helper.String());
		string_for_frame_of_day(helper, fSelectedScheduleItem->Duration());
		fDurationTC->SetText(helper.String());

		fFixedStartFrameCB->SetValue(
			!fSelectedScheduleItem->FlexibleStartFrame());
		bool flexibleDuration = fSelectedScheduleItem->FlexibleDuration();
		fFlexibleDurationCB->SetValue(flexibleDuration);
		_EnableControl(fRepeatsTC, !flexibleDuration);

		helper.SetTo("");
		if (!flexibleDuration)
			helper << fSelectedScheduleItem->ExplicitRepeats();
		else
			helper << fSelectedScheduleItem->Repeats();

		fRepeatsTC->SetText(helper.String());
	} else {
		_EnableControl(fRepeatsTC, false);
		fStartTimeTC->SetText("");
		fDurationTC->SetText("");
		fRepeatsTC->SetText("");

		fFixedStartFrameCB->SetValue(false);
		fFlexibleDurationCB->SetValue(false);
	}
}

// _EnableControl
template<class Control>
void
SchedulePropertiesView::_EnableControl(Control* control, bool enable)
{
	if (control->IsEnabled() != enable)
		control->SetEnabled(enable);
}



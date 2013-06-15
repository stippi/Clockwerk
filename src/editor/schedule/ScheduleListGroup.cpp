/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScheduleListGroup.h"

#include <stdio.h>
#include <stdlib.h>

#include <Menu.h>
#include <MenuBar.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <TextControl.h>

#include "support_date.h"

#include "Schedule.h"
#include "ScopeMenuField.h"
#include "Separator.h"
#include "WeekMenuItem.h"

enum {
	MSG_SET_WEEK				= 'stwk',
	MSG_SET_YEAR				= 'styr',
};

// constructor
ScheduleListGroup::ScheduleListGroup(ScheduleObjectListView* listView)
	:
	BView(BRect(0, 0, 300, 100), "schedule list group", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS),
	fScheduleListView(listView),
	fWeek(-2),
	fYear(0),
	fScope("")
{
	BRect frame = Bounds();
	frame.InsetBy(5, 5);
	fPreviousBounds = frame;

	frame.bottom = frame.top + 15;
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	BSeparatorView* separator = new BSeparatorView("List Filter Options");
	AddChild(separator);
	float width, height;
	separator->GetPreferredSize(&width, &height);
	separator->MoveTo(frame.LeftTop());
	separator->ResizeTo(frame.Width(), height);
	separator->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	// year text control
	frame.top = separator->Frame().bottom + 5;
	frame.bottom = frame.top + 15;
	fYearTC = new BTextControl(frame, "year", "Year", "",
		new BMessage(MSG_SET_YEAR), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);

	AddChild(fYearTC);
	fYearTC->GetPreferredSize(&width, &height);
	fYearTC->ResizeTo(frame.Width(), height);

	// type menu field
	fWeekM = new BPopUpMenu("weeks");

	frame.top = fYearTC->Frame().bottom + 5;
	frame.bottom = frame.top + 15;
	fWeekMF = new BMenuField(frame, "show week", "Week", fWeekM, true,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(fWeekMF);

	fWeekM->SetRadioMode(false);
	fWeekM->SetLabelFromMarked(false);

	// scope menu field
	BMenu* scopesM = new BPopUpMenu("<unset>");

	frame.top = fWeekMF->Frame().bottom + 5;
	frame.bottom = frame.top + 15;
	fScopeMF = new ScopeMenuField(frame, "show scopes", "Scope", scopesM,
		true, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	AddChild(fScopeMF);

	// alignment
	float divider = fWeekMF->StringWidth(fWeekMF->Label());
	divider = max_c(fYearTC->StringWidth(fYearTC->Label()), divider);
	divider = max_c(fScopeMF->StringWidth(fScopeMF->Label()), divider);

	fYearTC->SetDivider(divider + 4);
	fWeekMF->SetDivider(divider + 8);
	fScopeMF->SetDivider(divider + 8);

	fWeekMF->MenuBar()->GetPreferredSize(&width, &height);
	fWeekMF->MenuBar()->ResizeTo(width, height);
	fWeekMF->MenuBar()->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fWeekMF->ResizeTo(frame.Width(), height + 6);

	fScopeMF->MenuBar()->GetPreferredSize(&width, &height);
	fScopeMF->MenuBar()->ResizeTo(width, height);
	fScopeMF->MenuBar()->SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fScopeMF->ResizeTo(frame.Width(), height + 6);

	// clip list view
	frame = Bounds();
	frame.top = fScopeMF->Frame().bottom + 5;
	frame.right -= B_V_SCROLL_BAR_WIDTH + 1;
	fScheduleListView->MoveTo(frame.LeftTop());
	fScheduleListView->ResizeTo(frame.Width(), frame.Height());

	AddChild(new BScrollView("list scroll view", fScheduleListView,
		B_FOLLOW_ALL, 0, false, true, B_NO_BORDER));

	SetWeek(-1);

	time_t now = time(NULL);
	struct tm tm = *localtime(&now);
	SetYear(tm.tm_year + 1900);
}

// destructor
ScheduleListGroup::~ScheduleListGroup()
{
}

// AttachedToWindow
void
ScheduleListGroup::AttachedToWindow()
{
	_BuildWeekMenu(fWeekM);
	fYearTC->SetTarget(this);
	fScopeMF->Menu()->SetTargetForItems(this);
}

// AllAttached
void
ScheduleListGroup::AllAttached()
{
	SetViewColor(B_TRANSPARENT_COLOR);
}

// Draw
void
ScheduleListGroup::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenMax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

	BRect r(Bounds());
	r.bottom = fScheduleListView->Parent()->Frame().top - 1;

	BeginLineArray(4);
		AddLine(BPoint(r.left, r.bottom - 1),
				BPoint(r.left, r.top), lightenMax);
		AddLine(BPoint(r.left + 1, r.top),
				BPoint(r.right - 1, r.top), lightenMax);
		AddLine(BPoint(r.right, r.top),
				BPoint(r.right, r.bottom), darken2);
		AddLine(BPoint(r.right - 1, r.bottom),
				BPoint(r.left, r.bottom), darken2);
	EndLineArray();

	r.InsetBy(1, 1);
	r = r & updateRect;

	SetHighColor(base);
	FillRect(r);
}

// FrameResized
void
ScheduleListGroup::FrameResized(float width, float height)
{
	BRect dirty = fPreviousBounds;
	fPreviousBounds = Bounds();
	dirty = dirty & fPreviousBounds;
	dirty.left = dirty.right - 20;

	// GRRRR!!!!
	Invalidate(dirty);
	fWeekMF->ConvertFromParent(&dirty);
	fWeekMF->Invalidate(dirty);
	fWeekMF->MenuBar()->ConvertFromParent(&dirty);
	fWeekMF->MenuBar()->Invalidate(dirty);
}

// MessageReceived
void
ScheduleListGroup::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_SET_WEEK: {
			int32 week;
			if (message->FindInt32("week", &week) == B_OK)
				SetWeek(week);
			break;
		}
		case MSG_SET_YEAR: {
			int32 year = atoi(fYearTC->Text());
			if (year < 1970)
				year = 1970;
			SetYear(year);
			break;
		}
		case MSG_SET_SCOPE: {
			const char* scope;
			if (message->FindString("scop", &scope) == B_OK)
				SetScope(scope);
			break;
		}
		default:
			BView::MessageReceived(message);
	}
}

// #pragma mark -

// SetWeek
void
ScheduleListGroup::SetWeek(int32 week)
{
	if (week == fWeek)
		return;

	fWeek = week;

	fScheduleListView->SetWeek(week);

	if (BMenuItem* item = fWeekM->Superitem()) {
		BString label;
		if (week == -1)
			label = "All";
		else {
			label << week + 1 << ": ";
			uint16 firstDay = day_in_year(week, fYear);
			uint16 lastDay = day_in_year(week + 1, fYear) - 1;
			uint8 day, month;
			get_date(firstDay, fYear, day, month);
			label << day + 1 << "." << month + 1 << ".-";
			get_date(lastDay, fYear, day, month);
			label << day + 1 << "." << month + 1 << ".";
		}
		item->SetLabel(label.String());
	}
}

// SetYear
void
ScheduleListGroup::SetYear(int32 year)
{
	if (year == fYear)
		return;

	fYear = year;

	fScheduleListView->SetYear(year);

	BString yearLabel;
	yearLabel << fYear;
	fYearTC->SetText(yearLabel.String());
	fYearTC->TextView()->SelectAll();

	_BuildWeekMenu(fWeekM);
}

// SetScope
void
ScheduleListGroup::SetScope(const char* scope)
{
	if (fScope == scope)
		return;

	fScope = scope;

	fScheduleListView->SetScope(scope);

	fScopeMF->MarkScope(scope);
}

// SetScopes
void
ScheduleListGroup::SetScopes(const BMessage* scopes)
{
	fScopeMF->SetScopes(scopes);

	if (fScope == "")
		SetScope("all");
}


// #pragma mark -


// _CreateMonthMenu
BMenu*
ScheduleListGroup::_CreateMonthMenu(int32 monthIndex,
	const char* monthName) const
{
	BMenu* menu = new BMenu(monthName);

	// add week day header item
	menu->AddItem(new WeekMenuItem(NULL, -1, 0, fYear));

	// figure out weeks for this month
	int32 firstWeekIndex = week_in_year(0, monthIndex, fYear);
	int32 lastWeekIndex = week_in_year(days_in_month(monthIndex, fYear) - 1,
		monthIndex, fYear);

	for (int32 week = firstWeekIndex; week <= lastWeekIndex; week++) {
		BMessage* message = new BMessage(MSG_SET_WEEK);
		message->AddInt32("week", week);
		WeekMenuItem* item = new WeekMenuItem(message, week, monthIndex, fYear);
		menu->AddItem(item);
	}
	menu->SetTargetForItems(this);

	return menu;
}

// _BuildWeekMenu
void
ScheduleListGroup::_BuildWeekMenu(BMenu* menu) const
{
	while (BMenuItem* item = menu->RemoveItem(0L))
		delete item;

	// items
	BMenuItem* item;
	BMessage* message;

	message = new BMessage(MSG_SET_WEEK);
	message->AddInt32("week", -1);
	item = new BMenuItem("All", message);
	menu->AddItem(item);

	menu->AddSeparatorItem();

	menu->AddItem(_CreateMonthMenu(0, "January"));
	menu->AddItem(_CreateMonthMenu(1, "Februrary"));
	menu->AddItem(_CreateMonthMenu(2, "March"));
	menu->AddItem(_CreateMonthMenu(3, "April"));
	menu->AddItem(_CreateMonthMenu(4, "May"));
	menu->AddItem(_CreateMonthMenu(5, "June"));
	menu->AddItem(_CreateMonthMenu(6, "July"));
	menu->AddItem(_CreateMonthMenu(7, "August"));
	menu->AddItem(_CreateMonthMenu(8, "September"));
	menu->AddItem(_CreateMonthMenu(9, "October"));
	menu->AddItem(_CreateMonthMenu(10, "November"));
	menu->AddItem(_CreateMonthMenu(11, "December"));

	menu->SetTargetForItems(this);
}


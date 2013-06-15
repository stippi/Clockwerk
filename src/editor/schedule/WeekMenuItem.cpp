/*
 * Copyright 2005-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2007, Stephan Aßmus, superstippi@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include "WeekMenuItem.h"

#include <stdio.h>

#include <String.h>

#include "support_date.h"


static const int32 kTitleFontSize = 9;

static const int32 kTitleGap = 3;
static const int32 kColumnGap = 9;
static const int32 kRowGap = 2;


// constructor
WeekMenuItem::WeekMenuItem(BMessage* message,
		int32 week, int32 month, int32 year)
	: BMenuItem("week", message)
	, fWeek(week)
	, fMonth(month)
	, fYear(year)
{
	SetEnabled(fWeek >= 0);
		// used as the header if week < 0
}

// destructor
WeekMenuItem::~WeekMenuItem()
{
}

// DrawContent
void
WeekMenuItem::DrawContent()
{
	Menu()->PushState();
	BPoint origin = ContentLocation();

	rgb_color dayColor = Menu()->HighColor();

	BFont font;
	Menu()->GetFont(&font);
	font.SetSize(kTitleFontSize);
	Menu()->SetFont(&font, B_FONT_SIZE);

	float width;
	char text[64];

	if (fWeek < 0) {
		// Draw weekdays
		rgb_color titleColor = tint_color(Menu()->LowColor(), B_DARKEN_2_TINT);
		Menu()->SetHighColor(titleColor);
	
		time_t currentTime = time(NULL);
		struct tm tm;
		localtime_r(&currentTime, &tm);
	
		for (tm.tm_wday = 0; tm.tm_wday < 7; tm.tm_wday++) {
			strftime(text, sizeof(text), "%a", &tm);
	
			width = Menu()->StringWidth(text);
			Menu()->DrawString(text, BPoint(fColumnWidth * tm.tm_wday
				+ (fColumnWidth - width) / 2, fTitleHeight) + origin);
		}
		Menu()->PopState();
		return;
	} else {
		// Draw week
		rgb_color titleColor = tint_color(Menu()->LowColor(), B_DARKEN_4_TINT);
		Menu()->SetHighColor(titleColor);

		BString label("Week ");
		label << fWeek + 1;
		width = Menu()->StringWidth(label.String());
		Menu()->DrawString(label.String(), BPoint((fColumnWidth * 7 - width) / 2,
			fTitleHeight) + origin);
	}

	// Draw days

	Menu()->SetHighColor(dayColor);
	Menu()->SetFont(be_plain_font);

	Menu()->GetFont(&font);
	font.SetFace(B_BOLD_FACE);

	uint16 firstYearDayByWeek = day_in_year(fWeek, fYear);
	uint16 lastYearDayByWeek = day_in_year(fWeek + 1, fYear) - 1;

	uint16 firstYearDayByMonth = day_in_year(0, fMonth, fYear);
	uint16 lastYearDayByMonth = day_in_year(days_in_month(fMonth, fYear) - 1,
		fMonth, fYear);

	int32 firstDay = max_c(firstYearDayByWeek, firstYearDayByMonth);
	int32 lastDay = min_c(lastYearDayByWeek, lastYearDayByMonth);

	uint8 weekDayOffset = week_day(firstYearDayByMonth, fYear);

	firstDay -= firstYearDayByMonth;
	lastDay -= firstYearDayByMonth;

	for (int32 i = firstDay; i <= lastDay; i++) {
		int32 column = (i + weekDayOffset) % 7;

		sprintf(text, "%ld", i + 1);
		width = Menu()->StringWidth(text);
		BPoint point(fColumnWidth * column + (fColumnWidth - width) / 2,
			fTitleHeight + fRowHeight - kRowGap);

		Menu()->DrawString(text, point + origin);
	}

	Menu()->PopState();
}

// GetContentSize
void
WeekMenuItem::GetContentSize(float *_width, float *_height)
{
	BFont font;
	font.SetSize(kTitleFontSize);
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	fTitleHeight = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading);

	font = be_plain_font;
	font.GetHeight(&fontHeight);
	fRowHeight = ceilf(fontHeight.ascent + fontHeight.descent
		+ fontHeight.leading + kRowGap);
	fFontHeight = ceilf(fontHeight.ascent);
	fColumnWidth = font.StringWidth("99") + kColumnGap;

	if (_width)
		*_width = 7 * (fColumnWidth - 1);
	if (_height)
		*_height = fWeek < 0 ? fTitleHeight + kTitleGap
			: fRowHeight + fTitleHeight + kTitleGap;
}


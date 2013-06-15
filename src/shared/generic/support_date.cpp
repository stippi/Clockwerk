/*
 * Copyright 2001-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "support_date.h"

#include <math.h>
#include <parsedate.h>
#include <stdio.h>


static const int kDaysInMonth[12] = 
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const int kFirstDayOfMonth[2][12] = 
	{
		{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
		{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335 } 
	};


// is_leap_year
bool
is_leap_year(int32 year)
{
	return (year % 400 == 0) || (year % 4 == 0 && year % 100 != 0);
}

// days_in_month
uint8
days_in_month(uint8 month, int32 year)
{
	if (month == 1 && is_leap_year(year))
		return 29;
	
	return kDaysInMonth[month];
}

// day_in_year
uint16
day_in_year(uint8 day, uint8 month, int32 year)
{

	if (is_leap_year(year))
		return kFirstDayOfMonth[1][month] + day;
	else
		return kFirstDayOfMonth[0][month] + day;
}

// day_in_year
uint16
day_in_year(uint8 week, int32 year)
{
	if (week > 0) {
		uint16 dayInYear = week * 7;
		dayInYear -= week_day(0, 0, year);
		return dayInYear;
	}
	return 0;
}

// get_date
void
get_date(uint16 yearDay, int32 year, uint8& day, uint8& month)
{
	month = month_in_year(yearDay, year);
	day = yearDay - day_in_year(0, month, year);
	uint8 maxDay = days_in_month(month, year) - 1;
	if (day > maxDay)
		day = maxDay;
	
}

// month_in_year
uint8
month_in_year(uint16 yearDay, int32 year)
{
	uint8 month = 0;
	int32 monthLengthIndex = is_leap_year(year) ? 1 : 0;
	while (month < 11 && kFirstDayOfMonth[monthLengthIndex][month + 1] <= yearDay)
		month++;
	return month;
}

// week_in_year
uint8
week_in_year(uint8 day, uint8 month, int32 year)
{
	uint16 dayInYear = day_in_year(day, month, year);
	dayInYear += week_day(0, year);
	return dayInYear / 7;
}

// days_from_1970_to_year
int64
days_from_1970_to_year(int32 year)
{
	return (int64)(365.0 * (year - 1970)
		+ floor((year - 1969) / 4.0)
		- floor((year - 1901) / 100.0)
		+ floor((year - 1601) / 400.0));
}

// week_day
uint8
week_day(uint8 day, uint8 month, int32 year)
{
	// year is assumed to be later than 1970
	int64 daysSince1970 = days_from_1970_to_year(year);
	daysSince1970 += day_in_year(day, month, year);

	int weekDay = (daysSince1970 + 4) % 7;
	if (weekDay < 0)
		weekDay += 7;
	return weekDay;
}

// week_day
uint8
week_day(uint16 yearDay, int32 year)
{
	// year is assumed to be later than 1970
	int64 daysSince1970 = days_from_1970_to_year(year);
	daysSince1970 += yearDay;

	int weekDay = (daysSince1970 + 4) % 7;
	if (weekDay < 0)
		weekDay += 7;
	return weekDay;
}

// unix_time_for_week
time_t
unix_time_for_week(uint8 week, int32 year)
{
	if (week > 0) {
		int64 days = week * 7;
		days -= week_day(0, 0, year);
		days += days_from_1970_to_year(year);
		return (time_t)(days * 24 * 60 * 60);
	}
	return 0;
}

// #pragma mark -

// day_time_to_frame
int64
day_time_to_frame(const char* timeCodeString, float fps)
{
	int32 hours;
	int32 minutes;
	int32 seconds;
	int32 frames;

	if (sscanf(timeCodeString, "%ld:%ld:%ld.%ld",
		&hours, &minutes, &seconds, &frames) == 4) {
		// nothing to do
	} else if (sscanf(timeCodeString, "%ld:%ld:%ld",
		&hours, &minutes, &seconds) == 3) {
		frames = 0;
	} else if (sscanf(timeCodeString, "%ld:%ld",
		&hours, &minutes) == 2) {
		seconds = 0;
		frames = 0;
	} else {
		return -1;
	}

	// sanity check values
	if (minutes < 0 || minutes > 59
		|| seconds < 0 || seconds > 59
		|| frames < 0 || frames > fps)
		return -1;

	return (int64)floorf((hours * 60 * 60 + minutes * 60 + seconds)
		* fps + 0.5) + frames;
}

// string_for_frame
BString
string_for_frame(uint64 frames)
{
	int32 seconds = frames / 25;
	frames -= seconds * 25;
	int32 hours = seconds / (60 * 60);
	seconds -= hours * 60 * 60;
	int32 minutes = seconds / 60;
	seconds -= minutes * 60;
	BString string;
	string << hours << ':';
	if (minutes < 10)
		string << '0' << minutes << ':';
	else
		string << minutes << ':';
	if (seconds < 10)
		string << '0' << seconds << '.';
	else
		string << seconds << '.';
	if (frames < 10)
		string << '0' << frames;
	else
		string << frames;
	return string;
}

// day_diff_to_range
int32
day_diff_to_range(const char* day, const char* rangeStartDay,
	int32 rangeDayCount)
{
	if (!day || !rangeStartDay || rangeDayCount <= 0)
		return INT_MIN;

//printf("day_diff_to_range(%s, %s, %ld)\n", day, rangeStartDay, rangeDayCount);

	// get the current time/day (at 0:00am)
	time_t nowSeconds = time(NULL);
	tm now = *localtime(&nowSeconds);
	now.tm_sec = 0;
	now.tm_min = 0;
	now.tm_hour = 0;

	// convert "nowSeconds" to local time
	nowSeconds = mktime(&now);

	// get the range start date time/day (at 0:00am)
	time_t daySeconds = parsedate(day, nowSeconds);
	tm date = *localtime(&daySeconds);
	date.tm_sec = 0;
	date.tm_min = 0;
	date.tm_hour = 0;

	// convert "daySeconds" to local time
	daySeconds = mktime(&date);
//printf("day: %d.%d.%d\n", date.tm_mday, date.tm_mon, date.tm_year);

	// get the range start date time/day (at 0:00am)
	time_t rangeStartSeconds = parsedate(rangeStartDay, nowSeconds);
	tm rangeStartDate = *localtime(&rangeStartSeconds);
	rangeStartDate.tm_sec = 0;
	rangeStartDate.tm_min = 0;
	rangeStartDate.tm_hour = 0;

	// convert "rangeStartSeconds" to local time
	rangeStartSeconds = mktime(&rangeStartDate);
//printf("range: %d.%d.%d\n", rangeStartDate.tm_mday, rangeStartDate.tm_mon,
//	rangeStartDate.tm_year);

	time_t dayDuration = 60 * 60 * 24;
	time_t rangeDurationSeconds = dayDuration * rangeDayCount;

	int32 diffSeconds = 0;
		// init value to indicate day is within range

//printf("day: %d\n", daySeconds / dayDuration);
//printf("range start: %d\n", rangeStartSeconds / dayDuration);
//printf("range end: %d\n", (rangeStartSeconds + rangeDurationSeconds) / dayDuration);

	// test if day is outside range
	// if day is after range -> positive diff
	// if day is before range -> negative diff
	if (daySeconds > (rangeStartSeconds + rangeDurationSeconds))
		diffSeconds = daySeconds - (rangeStartSeconds + rangeDurationSeconds);
	else if (daySeconds < rangeStartSeconds)
		diffSeconds = daySeconds - rangeStartSeconds;

	return diffSeconds / dayDuration;
}


// today_is_within_range
bool
today_is_within_range(const char* startDateString, int32 dayCount)
{
	return day_diff_to_range("today", startDateString, dayCount) == 0;
}



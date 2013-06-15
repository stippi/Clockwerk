/*
 * Copyright 2001-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SUPPORT_DATE_H
#define SUPPORT_DATE_H

#include <time.h>

#include <String.h>


bool	is_leap_year(int32 year);

uint8	days_in_month(uint8 month, int32 year);

uint16	day_in_year(uint8 day, uint8 month, int32 year);
uint16	day_in_year(uint8 week, int32 year);
void	get_date(uint16 yearDay, int32 year, uint8& day, uint8& month);

uint8	month_in_year(uint16 yearDay, int32 year);

uint8	week_in_year(uint8 day, uint8 month, int32 year);

int64	days_from_1970_to_year(int32 year);

uint8	week_day(uint8 day, uint8 month, int32 year);
uint8	week_day(uint16 yearDay, int32 year);

time_t	unix_time_for_week(uint8 week, int32 year);

int64	day_time_to_frame(const char* timeCodeString, float fps = 25.0);

BString	string_for_frame(uint64 frames);

bool	today_is_within_range(const char* startDateString, int32 dayCount);
int32	day_diff_to_range(const char* day, const char* rangeStartDay,
			int32 rangeDayCount);


#endif // SUPPORT_DATE_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClockClip.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
#include <MessageFilter.h>

#include "common_constants.h"
#include "ui_defines.h"

#include "ColorProperty.h"
#include "CommonPropertyIDs.h"
#include "FontProperty.h"
#include "MessageConstants.h"
#include "OptionProperty.h"
#include "Property.h"

using std::nothrow;

// constructor
ClockClip::ClockClip(const char* name)
	: Clip("ClockClip", name),

	  fAnalogFace(dynamic_cast<BoolProperty*>(
			FindProperty(PROPERTY_CLOCK_ANALOG_FACE))),
	  fTimeFormat(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_TIME_FORMAT))),
	  fDateFormat(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_DATE_FORMAT))),
	  fDateOffset(dynamic_cast<IntProperty*>(
			FindProperty(PROPERTY_DATE_OFFSET))),

	  fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT))),
	  fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE))),
	  fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR))),
	  fGlyphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_GLYPH_SPACING))),

	  fLastCheck(0LL)
{
	fTime = time(NULL);
}

// constructor
ClockClip::ClockClip(const ClockClip& other)
	: Clip(other, true),

	  fAnalogFace(dynamic_cast<BoolProperty*>(
			FindProperty(PROPERTY_CLOCK_ANALOG_FACE))),
	  fTimeFormat(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_TIME_FORMAT))),
	  fDateFormat(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_DATE_FORMAT))),
	  fDateOffset(dynamic_cast<IntProperty*>(
			FindProperty(PROPERTY_DATE_OFFSET))),

	  fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT))),
	  fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE))),
	  fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR))),
	  fGlyphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_GLYPH_SPACING))),

	  fLastCheck(other.fLastCheck)
{
	fTime = time(NULL);
}

// destructor
ClockClip::~ClockClip()
{
}

// Duration
uint64
ClockClip::Duration()
{
	// TODO: ...
	return 300;
}

// Bounds
BRect
ClockClip::Bounds(BRect canvasBounds)
{
	float width = 99.0;
	float height = 99.0;

	if (!AnalogFace()) {
		::Font font = Font();
		font.SetSize(FontSize());
		BString text = Text();
		width = font.StringWidth(text.String());
	
		height = ceilf(font.Size());
	}

	BRect bounds;
	bounds.right = width;
	bounds.bottom = height;

	return bounds;
}

// #pragma mark -

// Time
time_t
ClockClip::Time()
{
	_UpdateTime(false);

	return fTime;
}

// Text
BString
ClockClip::Text()
{
	_UpdateTime(true);

	BString clockText;
	if (fDateString.Length() > 0) {
		clockText << fDateString;
		if (fTimeString.Length() > 0)
			clockText << ", ";
	}
	clockText << fTimeString;
	return clockText;
}

// AnalogFace
bool
ClockClip::AnalogFace() const
{
	if (fAnalogFace)
		return fAnalogFace->Value();
	return false;
}

// TimeFormat
uint32
ClockClip::TimeFormat() const
{
	if (fTimeFormat)
		return fTimeFormat->Value();
	return TIME_HH_MM_SS;
}

// DateFormat
uint32
ClockClip::DateFormat() const
{
	if (fDateFormat)
		return fDateFormat->Value();
	return DATE_DD_MM_YYYY;
}

// ::Font
::Font
ClockClip::Font() const
{
	if (fFont)
		return fFont->Value();
	return ::Font(*be_bold_font);
}

// FontSize
float
ClockClip::FontSize() const
{
	if (fFontSize)
		return fFontSize->Value();
	return kDefaultFontSize;
}

// Color
rgb_color
ClockClip::Color() const
{
	if (fColor)
		return fColor->Value();
	return kWhite;
}

// GlyphSpacing
float
ClockClip::GlyphSpacing() const
{
	if (fGlyphSpacing)
		return fGlyphSpacing->Value();
	return 1.0;
}

// #pragma mark -

void
ClockClip::_UpdateTime(bool updateStrings)
{
	bigtime_t now = system_time();
	if (fLastCheck + 1000000 < now) {
		fTime = time(NULL);

		if (fDateOffset) {
			fTime += fDateOffset->Value() * 24 * 60 * 60;
		}

		if (updateStrings) {
			_GetCurrentTime();
			_GetCurrentDate();
		}
		fLastCheck = now;
	}
}

// _GetCurrentTime
void
ClockClip::_GetCurrentTime()
{
	char tmp[64];
	tm time = *localtime(&fTime);

	switch (TimeFormat()) {
		case TIME_NONE:
			tmp[0] = 0;
			break;
		case TIME_HH_MM:
			strftime(tmp, 64, "%H:%M", &time);
			break;

		case TIME_HH_MM_SS:
		default:
			strftime(tmp, 64, "%H:%M:%S", &time);
			break;
	}
	//	remove leading 0 from time when hour is less than 10
	const char *str = tmp;
	if (str[0] == '0')
		str++;

	fTimeString = str;
}

static inline void
add_weekday(BString& string, const tm& time)
{
	switch (time.tm_wday) {
		case 0:
			string << "Sonntag";
			break;
		case 1:
			string << "Montag";
			break;
		case 2:
			string << "Dienstag";
			break;
		case 3:
			string << "Mittwoch";
			break;
		case 4:
			string << "Donnerstag";
			break;
		case 5:
			string << "Freitag";
			break;
		case 6:
			string << "Samstag";
			break;
	}
}

static inline void
add_month(BString& string, const tm& time)
{
	switch (time.tm_mon) {
		case 0:
			string << "Januar";
			break;
		case 1:
			string << "Februar";
			break;
		case 2:
			string << "März";
			break;
		case 3:
			string << "April";
			break;
		case 4:
			string << "Mai";
			break;
		case 5:
			string << "Juni";
			break;
		case 6:
			string << "Juli";
			break;
		case 7:
			string << "August";
			break;
		case 8:
			string << "September";
			break;
		case 9:
			string << "Oktober";
			break;
		case 10:
			string << "November";
			break;
		case 11:
			string << "Dezember";
			break;
	}
}

// _GetCurrentTime
void
ClockClip::_GetCurrentDate()
{
	char tmp[128];
	tmp[0] = 0;
	tm time = *localtime(&fTime);

	fDateString = "";

	switch (DateFormat()) {
		case DATE_NONE:
			break;
		case DATE_DD_MM_YY:
			strftime(tmp, 128, "%d.%m.%y", &time);
			break;
		case DATE_DD_MM:
			strftime(tmp, 128, "%d.%m", &time);
			break;
		case DATE_WEEKDAY_DD_MM_YYYY:
			add_weekday(fDateString, time);
			strftime(tmp, 128, ", %d.%m.%Y", &time);
			break;
		case DATE_WEEKDAY_DD_MM_YY:
			add_weekday(fDateString, time);
			strftime(tmp, 128, ", %d.%m.%y", &time);
			break;
		case DATE_WEEKDAY_DD_MONTH_YYYY:
			add_weekday(fDateString, time);
			fDateString << ", " << time.tm_mday;
			fDateString << ". ";
			add_month(fDateString, time);
			strftime(tmp, 128, " %Y", &time);
			break;
		case DATE_DD:
			strftime(tmp, 128, "%d", &time);
			break;
		case DATE_WEEKDAY:
			add_weekday(fDateString, time);
			break;
		case DATE_MONTH:
			add_month(fDateString, time);
			break;
		case DATE_MONTH_ALL_UPPERS:
			add_month(fDateString, time);
			fDateString.ToUpper();
			break;

		case DATE_DD_MM_YYYY:
		default:
			strftime(tmp, 128, "%d.%m.%Y", &time);
			break;
	}

	//	remove leading 0 from date when month is less than 10 (MM/DD/YY)
	//  or remove leading 0 from date when day is less than 10 (DD/MM/YY)
	const char* str = tmp;
	if (str[0] == '0')
		str++;

	fDateString << str;
}



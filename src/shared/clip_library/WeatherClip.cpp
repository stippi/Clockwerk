/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "WeatherClip.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Autolock.h>

#include "common_constants.h"
#include "ui_defines.h"

#include "ColorProperty.h"
#include "CommonPropertyIDs.h"
#include "FontProperty.h"
#include "MessageConstants.h"
#include "OptionProperty.h"
#include "Property.h"
#include "WeatherStatus.h"

using std::nothrow;

// constructor
WeatherClip::WeatherClip(const char* name)
	: Clip("WeatherClip", name),

	  fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT))),
	  fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE))),
	  fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR))),
	  fGlyphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_GLYPH_SPACING))),

	  fLocation(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_METAR_LOCATION))),
	  fDisplayMode(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_WEATHER_DISPLAY_LAYOUT))),
	  fWeatherTime(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_WEATHER_TIME))),

	  fSkyCondition(WeatherStatus::SKY_NONE),
	  fPhenomenon(WeatherStatus::PHENOMENON_NONE),
	  fTemperature(0.0),

	  fLastUpdate(LONGLONG_MIN)
{
}

// constructor
WeatherClip::WeatherClip(const WeatherClip& other)
	: Clip(other, true),

	  fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT))),
	  fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE))),
	  fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR))),
	  fGlyphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_GLYPH_SPACING))),

	  fLocation(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_METAR_LOCATION))),
	  fDisplayMode(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_WEATHER_DISPLAY_LAYOUT))),
	  fWeatherTime(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_WEATHER_TIME))),

	  fSkyCondition(other.fSkyCondition),
	  fPhenomenon(other.fPhenomenon),
	  fTemperature(other.fTemperature),

	  fLastUpdate(other.fLastUpdate)

{
}

// destructor
WeatherClip::~WeatherClip()
{
}

// Duration
uint64
WeatherClip::Duration()
{
	// TODO: ...
	return 300;
}

// Bounds
BRect
WeatherClip::Bounds(BRect canvasBounds)
{
	::Font font = Font();
	font.SetSize(FontSize());

	float height = 1.2 * font.Size();

	BRect bounds;
	bounds.right = ceilf(height + 5 + font.StringWidth("XX°C"));
	bounds.bottom = ceilf(height);

	return bounds;
}

// ValueChanged
void
WeatherClip::ValueChanged(Property* property)
{
	// force immediate update in case location was changed
	if (fLocation && fLocation == property) {
		fLastUpdate = LONGLONG_MIN;
		_UpdateWeather();
	}

	Clip::ValueChanged(property);
}

// #pragma mark -

// SkyCondition
WeatherStatus::sky_condition
WeatherClip::SkyCondition()
{
	_UpdateWeather();

	return fSkyCondition;
}

// Phenomenon
WeatherStatus::phenomenon
WeatherClip::Phenomenon()
{
	_UpdateWeather();

	return fPhenomenon;
}

// Temperatur
float
WeatherClip::Temperature()
{
	_UpdateWeather();

	return fTemperature;
}

// ::Font
::Font
WeatherClip::Font() const
{
	if (fFont)
		return fFont->Value();
	return ::Font(*be_bold_font);
}

// FontSize
float
WeatherClip::FontSize() const
{
	if (fFontSize)
		return fFontSize->Value();
	return kDefaultFontSize;
}

// Color
rgb_color
WeatherClip::Color() const
{
	if (fColor)
		return fColor->Value();
	return kWhite;
}

// GlyphSpacing
float
WeatherClip::GlyphSpacing() const
{
	if (fGlyphSpacing)
		return fGlyphSpacing->Value();
	return 1.0;
}

// DisplayMode
int32
WeatherClip::DisplayMode() const
{
	if (fDisplayMode)
		return fDisplayMode->Value();
	return WEATHER_DISPLAY_LAYOUT_ICON_LEFT;
}

// WeatherTime
int32
WeatherClip::WeatherTime() const
{
	if (fWeatherTime)
		return fWeatherTime->Value();
	return WEATHER_FORMAT_NOW;
}

// #pragma mark -

// _UpdateWeather
void
WeatherClip::_UpdateWeather()
{
	if (!fLocation) {
		printf("WeatherClip::_UpdateWeather() - no location property\n");
		return;
	}

	// only try to fetch every second
	bigtime_t now = system_time();
	if (fLastUpdate + 1000000 > now)
		return;

	fLastUpdate = now;

	// extract the four letter metar location identifier from the
	// current location option identifier
	unsigned locationID = B_HOST_TO_BENDIAN_INT32(fLocation->Value());
	char locationString[5];
	sprintf(locationString, "%.4s", (const char*)&locationID);
	locationString[4] = 0;

	WeatherStatusManager* manager = WeatherStatusManager::Default();
	WeatherStatus* status
		= manager->FetchInformationFor(locationString);

	if (!status)
		return;

	BAutolock _(status);

	fSkyCondition = status->SkyCondition();
	fPhenomenon = status->Phenomenon();
	fTemperature = status->TemperatureCelsius();
}


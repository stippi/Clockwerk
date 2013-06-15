/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef WEATHER_CLIP_H
#define WEATHER_CLIP_H

#include <String.h>

#include "Clip.h"
#include "Font.h"
#include "WeatherStatus.h"

class BoolProperty;
class ColorProperty;
class FloatProperty;
class FontProperty;
class OptionProperty;

class WeatherClip : public Clip {
 public:
								WeatherClip(const char* name = NULL);
								WeatherClip(const WeatherClip& other);
	virtual						~WeatherClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	void				ValueChanged(Property* property);

	// WeatherClip
			WeatherStatus::sky_condition SkyCondition();
			WeatherStatus::phenomenon Phenomenon();
			float				Temperature();

			::Font				Font() const;
			float				FontSize() const;
			rgb_color			Color() const;
			float				GlyphSpacing() const;
			int32				DisplayMode() const;
			int32				WeatherTime() const;

 private:
			void				_UpdateWeather();

			FontProperty*		fFont;
			FloatProperty*		fFontSize;
			ColorProperty*		fColor;
			FloatProperty*		fGlyphSpacing;

			OptionProperty*		fLocation;
			OptionProperty*		fDisplayMode;
			OptionProperty*		fWeatherTime;

			WeatherStatus::sky_condition fSkyCondition;
			WeatherStatus::phenomenon fPhenomenon;
			float				fTemperature;

			bigtime_t			fLastUpdate;
};

#endif // WEATHER_CLIP_H

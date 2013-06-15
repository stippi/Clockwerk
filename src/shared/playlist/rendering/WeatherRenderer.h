/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef WEATHER_RENDERER_H
#define WEATHER_RENDERER_H

#include <String.h>

#include "ClipRenderer.h"
#include "Font.h"
#include "WeatherStatus.h"

class WeatherClip;

class WeatherRenderer : public ClipRenderer {
 public:
								WeatherRenderer(
									ClipPlaylistItem* item,
									WeatherClip* clip);
	virtual						~WeatherRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	void				Sync();

 private:
			void				_RenderClear(Painter* painter, double scale);
			void				_RenderBroken(Painter* painter, double scale);
			void				_RenderOvercast(Painter* painter, double scale);
			void				_RenderRaining(Painter* painter, double scale);
			void				_RenderSnowing(Painter* painter, double scale);

			WeatherClip*			fClip;

			WeatherStatus::sky_condition fSkyCondition;
			WeatherStatus::phenomenon fPhenomenon;
			float				fTemperature;

			Font				fFont;
			rgb_color			fColor;
			int32				fDisplayMode;
};

#endif // WEATHER_RENDERER_H

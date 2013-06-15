/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "WeatherRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Shape.h>

#include "common_constants.h"
#include "ui_defines.h"
#include "support.h"

#include "CommonPropertyIDs.h"
#include "Painter.h"
#include "WeatherClip.h"

using std::nothrow;

// #pragma mark -

// constructor
WeatherRenderer::WeatherRenderer(ClipPlaylistItem* item, WeatherClip* clip)
	: ClipRenderer(item, clip),
	  fClip(clip),

	  fSkyCondition(clip ? clip->SkyCondition() : WeatherStatus::SKY_NONE),
	  fPhenomenon(clip ? clip->Phenomenon() : WeatherStatus::PHENOMENON_NONE),
	  fTemperature(clip ? clip->Temperature() : 0.0),

	  fFont(clip ? clip->Font() : *be_bold_font),
	  fColor(clip ? clip->Color() : kWhite),
	  fDisplayMode(clip ? clip->DisplayMode()
	  	: WEATHER_DISPLAY_LAYOUT_ICON_LEFT)
{
	if (fClip)
		fClip->Acquire();

	fFont.SetSize(kDefaultFontSize);
}

// destructor
WeatherRenderer::~WeatherRenderer()
{
	if (fClip)
		fClip->Release();
}

// Generate
status_t
WeatherRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	painter->SetFont(&fFont);
	painter->SetColor(fColor);

	bool iconRendered = fDisplayMode == WEATHER_DISPLAY_LAYOUT_NO_ICON ?
		true : false;
		// pretend to have already rendered the sky icon
		// if we are not supposed to render it

	// scale the icons according to font size
	double scale = 1.2 * fFont.Size() / 42.0;
		// the icons were drawn on a 42 pixel grid

	BPoint iconOffset(B_ORIGIN);

	BString text;
	text << (int32)roundf(fTemperature) << "°C";
	int32 length = strlen(text.String());
	BPoint offset(fFont.Size() * 1.2 + 5, 0);

	if (fDisplayMode == WEATHER_DISPLAY_LAYOUT_ICON_RIGHT) {
		offset.x = 0.0;
		iconOffset.x += painter->StringWidth(text.String(), length) + 5.0;
	}

	AffineTransform t;
	t.TranslateBy(iconOffset);
	painter->PushState();
	painter->SetTransformation(t);

	if (!iconRendered) {
		switch (fPhenomenon) {
			case WeatherStatus::PHENOMENON_DRIZZLE:
			case WeatherStatus::PHENOMENON_RAIN:
				_RenderRaining(painter, scale);
				iconRendered = true;
				break;
	
			case WeatherStatus::PHENOMENON_SNOW:
			case WeatherStatus::PHENOMENON_SNOW_GRAINS:
			case WeatherStatus::PHENOMENON_ICE_CRYSTALS:
			case WeatherStatus::PHENOMENON_ICE_PELLETS:
			case WeatherStatus::PHENOMENON_HAIL:
			case WeatherStatus::PHENOMENON_SMALL_HAIL:
			case WeatherStatus::PHENOMENON_UNKNOWN_PRECIPITATION:
				_RenderSnowing(painter, scale);
				iconRendered = true;
				break;
			
			case WeatherStatus::PHENOMENON_MIST:
			case WeatherStatus::PHENOMENON_FOG:
			case WeatherStatus::PHENOMENON_SMOKE:
			case WeatherStatus::PHENOMENON_VOLCANIC_ASH:
			case WeatherStatus::PHENOMENON_SAND:
			case WeatherStatus::PHENOMENON_HAZE:
			case WeatherStatus::PHENOMENON_SPRAY:
			case WeatherStatus::PHENOMENON_DUST:
			
			case WeatherStatus::PHENOMENON_SQUALL:
			case WeatherStatus::PHENOMENON_SANDSTORM:
			case WeatherStatus::PHENOMENON_DUSTSTORM:
			case WeatherStatus::PHENOMENON_FUNNEL_CLOUD:
			case WeatherStatus::PHENOMENON_TORNADO:
			case WeatherStatus::PHENOMENON_DUST_WHIRLS:
	
			case WeatherStatus::PHENOMENON_NONE:
			default:
				break;
		};
	}

	if (!iconRendered) {
		switch (fSkyCondition) {
			case WeatherStatus::SKY_BROKEN:
			case WeatherStatus::SKY_SCATTERED:
			case WeatherStatus::SKY_FEW:
				_RenderBroken(painter, scale);
				break;
			case WeatherStatus::SKY_OVERCAST:
				_RenderOvercast(painter, scale);
				break;

			case WeatherStatus::SKY_CLEAR:
			case WeatherStatus::SKY_NONE:
			default:
				_RenderClear(painter, scale);
				break;
		}
	}

	painter->PopState();

	font_height fh;
	fFont.GetHeight(&fh);
	offset.y += roundf((fFont.Size() * 1.2 - fh.ascent - fh.descent) / 2
		+ fh.ascent);
	painter->DrawString(text.String(), length, offset);

	return B_OK;
}

// Sync
void
WeatherRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip) {
		fSkyCondition = fClip->SkyCondition();
		fPhenomenon = fClip->Phenomenon();
		fTemperature = fClip->Temperature();
		fColor = fClip->Color();
		fFont = fClip->Font();
		fFont.SetSize(fClip->FontSize());
		fDisplayMode = fClip->DisplayMode();
	}
}

// #pragma mark -

static inline BPoint
scale_no_center(double x, double y, double scale)
{
	return BPoint(x * scale, y * scale);
}

static inline BPoint
scale_center(double x, double y, double scale, double offset)
{
	return BPoint(floorf(x * scale + 0.5) + offset,
		floorf(y * scale + 0.5) + offset);
}

static inline BPoint
scale_center_v(double x, double y, double scale, double offset)
{
	return BPoint(x * scale, floorf(y * scale + 0.5) + offset);
}

static inline BPoint
scale_center_h(double x, double y, double scale, double offset)
{
	return BPoint(floorf(x * scale + 0.5) + offset, y * scale);
}

// _RenderClear
void
WeatherRenderer::_RenderClear(Painter* painter, double scale)
{
	painter->SetSubpixelPrecise(true);
	painter->BeginComplexShape();
	int32 penSize = (int32)roundf(5.0 * scale);
	painter->SetPenSize(penSize);
	painter->SetLineMode(B_ROUND_CAP);

	double offset = (penSize % 2) * 0.5;

	BShape shape;
	shape.MoveTo(scale_center(20.5, 12.5, scale, offset));

	BPoint curve[3];
	curve[0] = scale_center_v(16.08, 12.5, scale, offset);
	curve[1] = scale_center_h(12.5, 16.08, scale, offset);
	curve[2] = scale_center(12.5, 20.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center_h(12.5, 24.91, scale, offset);
	curve[1] = scale_center_v(16.08, 28.5, scale, offset);
	curve[2] = scale_center(20.5, 28.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center_v(24.91, 28.5, scale, offset);
	curve[1] = scale_center_h(28.5, 24.91, scale, offset);
	curve[2] = scale_center(28.5, 20.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center_h(28.5, 16.08, scale, offset);
	curve[1] = scale_center_v(24.91, 12.5, scale, offset);
	curve[2] = scale_center(20.5, 12.5, scale, offset);
	shape.BezierTo(curve);

	shape.Close();

	painter->DrawShape(&shape, false);

	shape.Clear();

	shape.MoveTo(scale_center(20.5, 1.5, scale, offset));
	shape.LineTo(scale_center(20.5, 6.5, scale, offset));

	shape.MoveTo(scale_center(20.5, 34.5, scale, offset));
	shape.LineTo(scale_center(20.5, 39.5, scale, offset));

	shape.MoveTo(scale_center(1.5, 20.5, scale, offset));
	shape.LineTo(scale_center(6.5, 20.5, scale, offset));

	shape.MoveTo(scale_center(34.5, 20.5, scale, offset));
	shape.LineTo(scale_center(39.5, 20.5, scale, offset));

	shape.MoveTo(scale_center(6.5, 6.5, scale, offset));
	shape.LineTo(scale_center(10.5, 10.5, scale, offset));

	shape.MoveTo(scale_center(6.5, 34.5, scale, offset));
	shape.LineTo(scale_center(10.5, 30.5, scale, offset));

	shape.MoveTo(scale_center(34.5, 34.5, scale, offset));
	shape.LineTo(scale_center(30.5, 30.5, scale, offset));

	shape.MoveTo(scale_center(34.5, 6.5, scale, offset));
	shape.LineTo(scale_center(30.5, 10.5, scale, offset));

	painter->DrawShape(&shape, false);

	painter->EndComplexShape();
}

// _RenderBroken
void
WeatherRenderer::_RenderBroken(Painter* painter, double scale)
{
	painter->SetSubpixelPrecise(true);
	painter->BeginComplexShape();
	int32 penSize = (int32)roundf(5.0 * scale);
	painter->SetPenSize(penSize);
	painter->SetLineMode(B_ROUND_CAP);

	double offset = (penSize % 2) * 0.5;

	// sun
	BShape shape;
	shape.MoveTo(scale_center_h(18.5, 17, scale, offset));

	BPoint curve[3];
	curve[0] = scale_center_h(18.5, 13.96, scale, offset);
	curve[1] = scale_center_v(16.03, 11.5, scale, offset);
	curve[2] = scale_center_v(13, 11.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center_v(9.96, 11.5, scale, offset);
	curve[1] = scale_center_h(7.5, 13.96, scale, offset);
	curve[2] = scale_center_h(7.5, 17, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center_h(7.5, 18.76, scale, offset);
	curve[1] = scale_no_center(8.33, 20.33, scale);
	curve[2] = scale_no_center(9.62, 21.34, scale);
	shape.BezierTo(curve);

	painter->DrawShape(&shape, false);

	shape.Clear();

	shape.MoveTo(scale_center(12.5, 5.5, scale, offset));
	shape.LineTo(scale_center(12.5, 7.5, scale, offset));

	shape.MoveTo(scale_center(1.5, 16.5, scale, offset));
	shape.LineTo(scale_center(3.5, 16.5, scale, offset));

	shape.MoveTo(scale_center(4.5, 8.5, scale, offset));
	shape.LineTo(scale_center(5.5, 9.5, scale, offset));

	shape.MoveTo(scale_center(21.5, 8.5, scale, offset));
	shape.LineTo(scale_center(20.5, 9.5, scale, offset));

	painter->DrawShape(&shape, false);

	// cloud
	shape.Clear();

	shape.MoveTo(scale_center(10.5, 33.5, scale, offset));

	curve[0] = scale_center(14.5, 33.5, scale, offset);
	curve[1] = scale_center(29.5, 33.5, scale, offset);
	curve[2] = scale_center(32.5, 33.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(35.5, 33.5, scale, offset);
	curve[1] = scale_center(38.5, 30.5, scale, offset);
	curve[2] = scale_center(38.5, 27.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(38.5, 21.5, scale, offset);
	curve[1] = scale_center(30.5, 22.5, scale, offset);
	curve[2] = scale_center(30.5, 22.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(30.5, 22.5, scale, offset);
	curve[1] = scale_center(31.5, 16.5, scale, offset);
	curve[2] = scale_center(24.5, 15.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_no_center(16.58, 14.36, scale);
	curve[1] = scale_center(15.5, 21.5, scale, offset);
	curve[2] = scale_center(15.5, 21.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(15.5, 21.5, scale, offset);
	curve[1] = scale_center(2.5, 20.5, scale, offset);
	curve[2] = scale_center(2.5, 27.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(2.5, 32.5, scale, offset);
	curve[1] = scale_center(6.5, 33.5, scale, offset);
	curve[2] = scale_center(10.5, 33.5, scale, offset);
	shape.BezierTo(curve);

	shape.Close();

	painter->DrawShape(&shape, false);
	painter->EndComplexShape();
}

// _RenderOvercast
void
WeatherRenderer::_RenderOvercast(Painter* painter, double scale)
{
	painter->SetSubpixelPrecise(true);
	painter->BeginComplexShape();
	int32 penSize = (int32)roundf(5.0 * scale);
	painter->SetPenSize(penSize);
	painter->SetLineMode(B_ROUND_CAP);

	double offset = (penSize % 2) * 0.5;

	// cloud
	BShape shape;
	shape.MoveTo(scale_center(10.5, 31.5, scale, offset));

	BPoint curve[3];
	curve[0] = scale_center(14.5, 31.5, scale, offset);
	curve[1] = scale_center(29.5, 31.5, scale, offset);
	curve[2] = scale_center(32.5, 31.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(35.5, 31.5, scale, offset);
	curve[1] = scale_center(38.5, 28.5, scale, offset);
	curve[2] = scale_center(38.5, 25.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(38.5, 19.5, scale, offset);
	curve[1] = scale_center(30.5, 20.5, scale, offset);
	curve[2] = scale_center(30.5, 20.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(30.5, 20.5, scale, offset);
	curve[1] = scale_center(29.5, 13.5, scale, offset);
	curve[2] = scale_center(21.5, 13.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(13.5, 13.5, scale, offset);
	curve[1] = scale_center(12.5, 20.5, scale, offset);
	curve[2] = scale_center(12.5, 20.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(12.5, 20.5, scale, offset);
	curve[1] = scale_center(2.5, 19.5, scale, offset);
	curve[2] = scale_center(2.5, 26.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(2.5, 31.5, scale, offset);
	curve[1] = scale_center(6.5, 31.5, scale, offset);
	curve[2] = scale_center(10.5, 31.5, scale, offset);
	shape.BezierTo(curve);

	shape.Close();

	painter->DrawShape(&shape, false);
	painter->EndComplexShape();
}

// _RenderRaining
void
WeatherRenderer::_RenderRaining(Painter* painter, double scale)
{
	painter->SetSubpixelPrecise(true);
	painter->BeginComplexShape();
	int32 penSize = (int32)roundf(5.0 * scale);
	painter->SetPenSize(penSize);
	painter->SetLineMode(B_ROUND_CAP);

	double offset = (penSize % 2) * 0.5;

	// cloud
	BShape shape;
	shape.MoveTo(scale_center(10.5, 22.5, scale, offset));

	BPoint curve[3];
	curve[0] = scale_center(14.5, 22.5, scale, offset);
	curve[1] = scale_center(29.5, 22.5, scale, offset);
	curve[2] = scale_center(32.5, 22.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(35.5, 22.5, scale, offset);
	curve[1] = scale_center(38.5, 19.5, scale, offset);
	curve[2] = scale_center(38.5, 16.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(38.5, 10.5, scale, offset);
	curve[1] = scale_center(30.5, 11.5, scale, offset);
	curve[2] = scale_center(30.5, 11.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(30.5, 11.5, scale, offset);
	curve[1] = scale_center(29.5, 4.5, scale, offset);
	curve[2] = scale_center(21.5, 4.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(13.5, 4.5, scale, offset);
	curve[1] = scale_center(12.5, 11.5, scale, offset);
	curve[2] = scale_center(12.5, 11.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(12.5, 11.5, scale, offset);
	curve[1] = scale_center(2.5, 10.5, scale, offset);
	curve[2] = scale_center(2.5, 17.5, scale, offset);
	shape.BezierTo(curve);

	curve[0] = scale_center(2.5, 22.5, scale, offset);
	curve[1] = scale_center(6.5, 22.5, scale, offset);
	curve[2] = scale_center(10.5, 22.5, scale, offset);
	shape.BezierTo(curve);

	shape.Close();

	painter->DrawShape(&shape, false);

	// rain
	shape.Clear();

	shape.MoveTo(scale_no_center(10.5, 23.5, scale));
	shape.LineTo(scale_no_center(12.5, 31.5, scale));

	shape.MoveTo(scale_no_center(17.5, 23.5, scale));
	shape.LineTo(scale_no_center(21.5, 38.5, scale));

	shape.MoveTo(scale_no_center(24.5, 23.5, scale));
	shape.LineTo(scale_no_center(27.5, 33.5, scale));

	shape.MoveTo(scale_no_center(31.5, 23.5, scale));
	shape.LineTo(scale_no_center(35.5, 35.5, scale));

	painter->DrawShape(&shape, false);

	painter->EndComplexShape();
}

// _RenderSnowing
void
WeatherRenderer::_RenderSnowing(Painter* painter, double scale)
{
	painter->SetSubpixelPrecise(true);
	painter->BeginComplexShape();
	int32 penSize = (int32)roundf(5.0 * scale);
	painter->SetPenSize(penSize);
	painter->SetLineMode(B_ROUND_CAP);

	double offset = (penSize % 2) * 0.5;

	// center
	BShape shape;
	shape.MoveTo(scale_center(20.5, 13.5, scale, offset));
	shape.LineTo(scale_center(13.5, 20.5, scale, offset));
	shape.LineTo(scale_center(20.5, 27.5, scale, offset));
	shape.LineTo(scale_center(27.5, 20.5, scale, offset));
	shape.Close();

	painter->DrawShape(&shape, false);

	// spikes
	shape.Clear();

	shape.MoveTo(scale_center(20.5, 13.5, scale, offset));
	shape.LineTo(scale_center(20.5, 3.5, scale, offset));

	shape.MoveTo(scale_center(13.5, 20.5, scale, offset));
	shape.LineTo(scale_center(3.5, 20.5, scale, offset));

	shape.MoveTo(scale_center(27.5, 20.5, scale, offset));
	shape.LineTo(scale_center(37.5, 20.5, scale, offset));

	shape.MoveTo(scale_center(20.5, 28.5, scale, offset));
	shape.LineTo(scale_center(20.5, 37.5, scale, offset));

	painter->DrawShape(&shape, false);

	// small spikes
	shape.Clear();

	shape.MoveTo(scale_center(8.5, 15.5, scale, offset));
	shape.LineTo(scale_center(8.5, 25.5, scale, offset));

	shape.MoveTo(scale_center(15.5, 8.5, scale, offset));
	shape.LineTo(scale_center(25.5, 8.5, scale, offset));

	shape.MoveTo(scale_center(32.5, 15.5, scale, offset));
	shape.LineTo(scale_center(32.5, 25.5, scale, offset));

	shape.MoveTo(scale_center(15.5, 32.5, scale, offset));
	shape.LineTo(scale_center(25.5, 32.5, scale, offset));

	painter->DrawShape(&shape, false);

	painter->EndComplexShape();
}

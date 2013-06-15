/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClockRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "common_constants.h"
#include "ui_defines.h"
#include "support.h"

#include "ClockClip.h"
#include "Painter.h"

using std::nothrow;

// #pragma mark -

#define CLOCK_RENDER_TIMING 0

// constructor
ClockRenderer::ClockRenderer(ClipPlaylistItem* item, ClockClip* clip)
	: ClipRenderer(item, clip)
	, fClip(clip)

	, fAnalogFace(clip ? clip->AnalogFace() : false)

	, fText(clip ? clip->Text() : BString("12:00:00"))
	, fFont(clip ? clip->Font() : *be_bold_font)
	, fColor(clip ? clip->Color() : kWhite)

#if CLOCK_RENDER_TIMING
	, fRenderTime(0)
	, fSetFontTime(0)
	, fGeneratedFrames(0)
#endif
{
	if (fClip)
		fClip->Acquire();

	fFont.SetSize(clip ? clip->FontSize() : kDefaultFontSize);

	fTime = time(NULL);
}

// destructor
ClockRenderer::~ClockRenderer()
{
	if (fClip)
		fClip->Release();

#if CLOCK_RENDER_TIMING
	if (fGeneratedFrames > 0) {
		printf("avg clock rendering: %lld (font: %lld)\n",
			   fRenderTime / fGeneratedFrames, fSetFontTime / fGeneratedFrames);
	}
#endif
}

// Generate
status_t
ClockRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	if (fAnalogFace)
		_RenderAnalogClock(painter, frame);
	else
		_RenderDigitalClock(painter, frame);

	return B_OK;
}

// Sync
void
ClockRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip) {
		fAnalogFace = fClip->AnalogFace();
		fColor = fClip->Color();
		if (fAnalogFace) {
			fTime = fClip->Time();
		} else {
			fText = fClip->Text();
			fFont = fClip->Font();
			fFont.SetSize(fClip->FontSize());
		}
	}
}

// #pragma mark -

// _RenderDigitalClock
void
ClockRenderer::_RenderDigitalClock(Painter* painter, double frame)
{
#if CLOCK_RENDER_TIMING
bigtime_t start = system_time();
#endif

	painter->SetFont(&fFont);
#if CLOCK_RENDER_TIMING
bigtime_t setFont = system_time();
#endif
	painter->SetSubpixelPrecise(false);
	painter->SetColor(fColor);

	// TODO: align right within maximum clock bounds
	BPoint offset(B_ORIGIN);
	font_height fh;
	fFont.GetHeight(&fh);
	offset.y += roundf((fFont.Size() * 1.2 - fh.ascent - fh.descent) / 2
		+ fh.ascent);

	painter->DrawString(fText.String(), fText.Length(), offset);

#if CLOCK_RENDER_TIMING
fRenderTime += system_time() - start;
fSetFontTime += setFont - start;
fGeneratedFrames++;
#endif
}

// _RenderAnalogClock
void
ClockRenderer::_RenderAnalogClock(Painter* painter, double frame)
{
	painter->SetSubpixelPrecise(true);
	painter->BeginComplexShape();
		// BeginComplexShape/EndComplexShape causes all
		// drawings to be rendered in one go instead of being
		// layered on top of each other, which is important
		// with regards to rendering the clock semi-transparent

	bool drawMinutesMarks = painter->Scale() >= 2.0;

	if (drawMinutesMarks) {
		// TODO: make this color configurable
		painter->SetColor(255, 0, 0, fColor.alpha);
	}

	// hour digits
	painter->SetPenSize(2.0);
	painter->SetLineMode(B_ROUND_CAP);
	for (int32 hour = 0; hour < 12; hour++) {
		float x1 = 50.0 + sinf(hour * 2 * M_PI / 12.0) * 50.0;
		float y1 = 50.0 + cosf(hour * 2 * M_PI / 12.0) * 50.0;
		float x2 = 50.0 + sinf(hour * 2 * M_PI / 12.0) * 45.0;
		float y2 = 50.0 + cosf(hour * 2 * M_PI / 12.0) * 45.0;
		painter->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
	}

	if (drawMinutesMarks) {
		// flush shape, because we switch color
		painter->EndComplexShape();
		painter->BeginComplexShape();
	}

	painter->SetColor(fColor);

	if (drawMinutesMarks) {
		painter->SetPenSize(1.0);
		painter->SetLineMode(B_BUTT_CAP);
		for (int32 minute = 1; minute < 60; minute++) {
			if (minute % 5 == 0)
				continue;
			float x1 = 50.0 + sinf(minute * 2 * M_PI / 60.0) * 50.0;
			float y1 = 50.0 + cosf(minute * 2 * M_PI / 60.0) * 50.0;
			float x2 = 50.0 + sinf(minute * 2 * M_PI / 60.0) * 48.5;
			float y2 = 50.0 + cosf(minute * 2 * M_PI / 60.0) * 48.5;
			painter->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));
		}
		painter->SetPenSize(2.0);
		painter->SetLineMode(B_ROUND_CAP);
	}

	tm time = *localtime(&fTime);

	// center
	float x1 = 50.0;
	float y1 = 50.0;
	painter->DrawEllipse(BPoint(x1, y1), 3.0, 3.0, true);

	// hour hand
	float hour = (time.tm_hour * 60.0 + time.tm_min) / 60.0;

	float x2 = 50.0 + sinf(hour * 2 * M_PI / 12.0) * 32.0;
	float y2 = 50.0 - cosf(hour * 2 * M_PI / 12.0) * 32.0;

	painter->SetPenSize(4.0);
	painter->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));

	// minute hand
	float minute = (time.tm_min * 60.0 + time.tm_sec) / 60.0;

	x2 = 50.0 + sinf(minute * 2 * M_PI / 60.0) * 46.0;
	y2 = 50.0 - cosf(minute * 2 * M_PI / 60.0) * 46.0;

	painter->SetPenSize(3.0);
	painter->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));

	// second hand
	float second = (float)time.tm_sec;

	x2 = 50.0 + sinf(second * 2 * M_PI / 60.0) * 44.0;
	y2 = 50.0 - cosf(second * 2 * M_PI / 60.0) * 44.0;

	if (drawMinutesMarks) {
		// flush shape, because we switch color
		painter->EndComplexShape();
		painter->BeginComplexShape();
		// TODO: make this color configurable
		painter->SetColor(255, 0, 0, fColor.alpha);
	}
	painter->SetPenSize(1.0);
	painter->StrokeLine(BPoint(x1, y1), BPoint(x2, y2));

	painter->EndComplexShape();
}

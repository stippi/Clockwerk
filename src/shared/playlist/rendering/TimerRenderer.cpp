/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TimerRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "common_constants.h"
#include "ui_defines.h"
#include "support.h"

#include "CommonPropertyIDs.h"
#include "TimerClip.h"
#include "Painter.h"
#include "RenderPlaylistItem.h"

using std::nothrow;

// constructor
TimerRenderer::TimerRenderer(ClipPlaylistItem* item,
		TimerClip* clip)
	: ClipRenderer(item, clip)
	, fClip(clip)

	, fFont(clip ? clip->Font() : *be_bold_font)
	, fColor(clip ? clip->Color() : kWhite)

	, fTimeFormat(0)
	, fTimerDirection()
{
	if (fClip)
		fClip->Acquire();

	fFont.SetSize(clip ? clip->FontSize() : kDefaultFontSize);
}

// destructor
TimerRenderer::~TimerRenderer()
{
	if (fClip)
		fClip->Release();
}

// Generate
status_t
TimerRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	painter->SetFont(&fFont);
	painter->SetSubpixelPrecise(false);
	painter->SetColor(fColor);

	// TODO: align right within maximum countdown bounds?
	BPoint offset(B_ORIGIN);
	font_height fh;
	fFont.GetHeight(&fh);
	offset.y += roundf((fFont.Size() * 1.2 - fh.ascent - fh.descent) / 2
		+ fh.ascent);

	double frames;
	if (fTimerDirection == TIMER_DIRECTION_FORWARD) {
		frames = frame;
	} else {
		frames = (item->Duration() - 1) - frame;
	}
	double second = frames / item->VideoFramesPerSecond();

	char text[64];

	switch (fTimeFormat) {
		case TIME_HH_MM_SS: {
			int32 hours = (int32)(second / (60 * 60));
			second -= hours * 60 * 60;
			int32 minutes = (int32)(second / 60);
			second -= minutes * 60;
			sprintf(text, "%ld:%.2ld:%.2ld", hours, minutes, (int32)second);
			break;
		}

		case TIME_MM_SS: {
			int32 minutes = (int32)(second / 60);
			second -= minutes * 60;
			sprintf(text, "%ld:%lld", minutes, (int64)second);
			break;
		}

		case TIME_SS:
		default: {
			sprintf(text, "%lld", (int64)second);
			break;
		}

		case TIME_HH_MM_SS_MMM: {
			int32 hours = (int32)(second / (60 * 60));
			second -= hours * 60 * 60;
			int32 minutes = (int32)(second / 60);
			second -= minutes * 60;
			int32 secondRest = (int32)((second - (int32)second) * 1000);
			sprintf(text, "%ld:%.2ld:%.2ld.%.3ld", hours, minutes,
				(int32)second, secondRest);
			break;
		}

		case TIME_MM_SS_MMM: {
			int32 minutes = (int32)(second / 60);
			second -= minutes * 60;
			int32 secondsRest = (int32)((second - (int32)second) * 1000);
			sprintf(text, "%.2ld:%.2ld.%.3ld", minutes, (int32)second,
				secondsRest);
			break;
		}

		case TIME_SS_MMM: {
			sprintf(text, "%.3f", second);
			break;
		}

		case TIME_HH_MM_SS_FF: {
			int32 hours = (int32)(second / (60 * 60));
			second -= hours * 60 * 60;
			int32 minutes = (int32)(second / 60);
			second -= minutes * 60;
			int32 secondsRest = (int32)fmod(frames,
				item->VideoFramesPerSecond());
			sprintf(text, "%ld:%.2ld:%.2ld.%.2ld", hours, minutes,
				(int32)second, secondsRest);
			break;
		}
	}
	
	painter->DrawString(text, strlen(text), offset);

	return B_OK;
}

// Sync
void
TimerRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip) {
		fColor = fClip->Color();
		fFont = fClip->Font();
		fFont.SetSize(fClip->FontSize());
		fTimeFormat = fClip->TimeFormat();
		fTimerDirection = fClip->TimerDirection();
	}
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TimerClip.h"

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
TimerClip::TimerClip(const char* name)
	: Clip("TimerClip", name),

	  fTimeFormat(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_TIME_FORMAT))),
	  fTimerDirection(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_TIMER_DIRECTION))),

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
}

// constructor
TimerClip::TimerClip(const TimerClip& other)
	: Clip(other, true),

	  fTimeFormat(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_TIME_FORMAT))),
	  fTimerDirection(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_TIMER_DIRECTION))),

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
}

// destructor
TimerClip::~TimerClip()
{
}

// Duration
uint64
TimerClip::Duration()
{
	// TODO: ...
	return 300;
}

// Bounds
BRect
TimerClip::Bounds(BRect canvasBounds)
{
	::Font font = Font();
	font.SetSize(FontSize());

	BRect bounds;
	// TODO: string should depend on format
	bounds.right = font.StringWidth("00:00:00.000");
	bounds.bottom = ceilf(font.Size());

	return bounds;
}

// #pragma mark -

// TimeFormat
uint32
TimerClip::TimeFormat() const
{
	if (fTimeFormat)
		return fTimeFormat->Value();
	return TIME_HH_MM_SS;
}

// TimerDirection
uint32
TimerClip::TimerDirection() const
{
	if (fTimerDirection)
		return fTimerDirection->Value();
	return TIMER_DIRECTION_BACKWARD;
}

// Font
::Font
TimerClip::Font() const
{
	if (fFont)
		return fFont->Value();
	return ::Font(*be_bold_font);
}

// FontSize
float
TimerClip::FontSize() const
{
	if (fFontSize)
		return fFontSize->Value();
	return kDefaultFontSize;
}

// Color
rgb_color
TimerClip::Color() const
{
	if (fColor)
		return fColor->Value();
	return kWhite;
}

// GlyphSpacing
float
TimerClip::GlyphSpacing() const
{
	if (fGlyphSpacing)
		return fGlyphSpacing->Value();
	return 1.0;
}


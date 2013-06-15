/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ColorClip.h"

#include <stdio.h>
#include <stdlib.h>

#include <Bitmap.h>

#include "ui_defines.h"

#include "BBitmapBuffer.h"
#include "CommonPropertyIDs.h"
#include "ColorProperty.h"
#include "Painter.h"

// constructor
ColorClip::ColorClip(const char* name)
	: Clip("ColorClip", name),
	  fColor(dynamic_cast<ColorProperty*>(FindProperty(PROPERTY_COLOR)))
{
}

// constructor
ColorClip::ColorClip(const ColorClip& other)
	: Clip(other, true),
	  fColor(dynamic_cast<ColorProperty*>(FindProperty(PROPERTY_COLOR)))
{
}

// destructor
ColorClip::~ColorClip()
{
}

// Duration
uint64
ColorClip::Duration()
{
	return 0;
}

// Bounds
BRect
ColorClip::Bounds(BRect canvasBounds)
{
	// color fills the entire canvas
	return canvasBounds;
}

// GetIcon
bool
ColorClip::GetIcon(BBitmap* icon)
{
	Painter painter;
	BBitmapBuffer buffer(icon);
	painter.AttachToBuffer(&buffer);
	painter.SetColor(fColor->Value());
	painter.FillRect(icon->Bounds());
	return true;
}

// #pragma mark -
rgb_color
ColorClip::Color() const
{
	if (fColor)
		return fColor->Value();

	return kBlack;
}


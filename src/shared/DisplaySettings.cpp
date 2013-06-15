/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DisplaySettings.h"

#include "CommonPropertyIDs.h"
#include "OptionProperty.h"
#include "Property.h"

// constructor
DisplaySettings::DisplaySettings()
	: ServerObject("DisplaySettings"),
	  fWidth(dynamic_cast<IntProperty*>(FindProperty(PROPERTY_WIDTH))),
	  fHeight(dynamic_cast<IntProperty*>(FindProperty(PROPERTY_HEIGHT)))
{
}

// destructor
DisplaySettings::~DisplaySettings()
{
}

// #pragma mark -

// SetWidth
void
DisplaySettings::SetWidth(int32 width)
{
	if (fWidth)
		fWidth->SetValue(width);
}

// Width
int32
DisplaySettings::Width() const
{
	if (fWidth)
		return fWidth->Value();

	return 640;
}

// SetHeight
void
DisplaySettings::SetHeight(int32 height)
{
	if (fHeight)
		fHeight->SetValue(fHeight);
}

// Height
int32
DisplaySettings::Height() const
{
	if (fHeight)
		return fHeight->Value();

	return 480;
}

// InputSource
int32
DisplaySettings::InputSource() const
{
	OptionProperty* property = dynamic_cast<OptionProperty*>(
		FindProperty(PROPERTY_INPUT_SOURCE));
	if (property == NULL)
		return DISPLAY_INPUT_SOURCE_VGA;

	return property->CurrentOptionID();
}

// DisplayWidth
int32
DisplaySettings::DisplayWidth() const
{
	IntProperty* property = dynamic_cast<IntProperty*>(
		FindProperty(PROPERTY_DISPLAY_WIDTH));
	if (property == NULL)
		return 1368;

	return property->Value();
}

// DisplayHeight
int32
DisplaySettings::DisplayHeight() const
{
	IntProperty* property = dynamic_cast<IntProperty*>(
		FindProperty(PROPERTY_DISPLAY_HEIGHT));
	if (property == NULL)
		return 1368;

	return property->Value();
}

// DisplayFrequency
float
DisplaySettings::DisplayFrequency() const
{
	FloatProperty* property = dynamic_cast<FloatProperty*>(
		FindProperty(PROPERTY_DISPLAY_FREQUENCY));
	if (property == NULL)
		return 768;

	return property->Value();
}

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Clip.h"

#include <stdio.h>

#include <Bitmap.h>
#include <OS.h>

#include "CommonPropertyIDs.h"
#include "Icons.h"
#include "Property.h"

// globals
int32 Clip::sUnamedClipCount = 0;

// constructor
Clip::Clip(const char* type, const char* name)
	: ServerObject(type),
	  Selectable()
{
	BString n;
	if (!name || strlen(name) == 0) {
		n = "<unnamed clip ";
		atomic_add(&sUnamedClipCount, 1);
		n << sUnamedClipCount << ">";
	} else
		n = name;

	SetName(n);
}

// constructor
Clip::Clip(const Clip& other, bool deep)
	: ServerObject(other, deep),
	  Selectable()
{
}

// destructor
Clip::~Clip()
{
//printf("Clip(%s)::~Clip()\n", Name().String());
}

// SelectedChanged
void
Clip::SelectedChanged()
{
	// do nothing (no notification necessary)
}

// MaxDuration
uint64
Clip::MaxDuration()
{
	return LONG_MAX;
}

// HasMaxDuration
bool
Clip::HasMaxDuration()
{
	return MaxDuration() < LONG_MAX;
}

// #pragma mark -

// HasVideo
bool
Clip::HasVideo()
{
	return true;
}

// HasAudio
bool
Clip::HasAudio()
{
	return false;
}

// CreateAudioReader
AudioReader*
Clip::CreateAudioReader()
{
	return NULL;
}

// LogPlayback
bool
Clip::LogPlayback() const
{
	return Value(PROPERTY_LOG_PLAYBACK, false);
}

// #pragma mark -

// SetTemplateName
void
Clip::SetTemplateName(const char* templateName)
{
	SetValue(PROPERTY_TEMPLATE_NAME, templateName);
}

// TemplateName
const char*
Clip::TemplateName() const
{
	return Value(PROPERTY_TEMPLATE_NAME, "");
}

// IsTemplate
bool
Clip::IsTemplate() const
{
	BString templateName(TemplateName());
	return templateName.Length() > 0;
}

// #pragma mark -

// GetIcon
bool
Clip::GetIcon(BBitmap* icon)
{
	return false;
}

// ChangeToken
uint32
Clip::ChangeToken() const
{
	return 0;
}

// GetBuiltInIcon
bool
Clip::GetBuiltInIcon(BBitmap* icon, const uchar* iconData) const
{
	BRect iconBounds = icon->Bounds();
	BRect iconDataBounds(0, 0, kItemIconWidth - 1, kItemIconHeight - 1);
	if (iconBounds != iconDataBounds || icon->ColorSpace() != kIconFormat)
		return false;

	memcpy(icon->Bits(), iconData, icon->BitsLength());

	return true;
}


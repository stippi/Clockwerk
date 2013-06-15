/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COUNTDOWN_CLIP_H
#define COUNTDOWN_CLIP_H

#include <String.h>

#include "Clip.h"
#include "Font.h"

class BMessageFilter;
class BoolProperty;
class ColorProperty;
class FloatProperty;
class FontProperty;
class IntProperty;
class OptionProperty;
class StringProperty;

class TimerClip : public Clip {
 public:
								TimerClip(const char* name = NULL);
								TimerClip(const TimerClip& other);
	virtual						~TimerClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	// TimerClip
			uint32				TimeFormat() const;
			uint32				TimerDirection() const;

			::Font				Font() const;
			float				FontSize() const;
			rgb_color			Color() const;
			float				GlyphSpacing() const;

 private:
			OptionProperty*		fTimeFormat;
			OptionProperty*		fTimerDirection;
			FontProperty*		fFont;
			FloatProperty*		fFontSize;
			ColorProperty*		fColor;
			FloatProperty*		fGlyphSpacing;

			bigtime_t			fLastCheck;
};

#endif // COUNTDOWN_CLIP_H

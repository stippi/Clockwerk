/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLOCK_CLIP_H
#define CLOCK_CLIP_H

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

class ClockClip : public Clip {
 public:
								ClockClip(const char* name = NULL);
								ClockClip(const ClockClip& other);
	virtual						~ClockClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	// ClockClip
			time_t				Time();
			BString				Text();

			bool				AnalogFace() const;
			uint32				TimeFormat() const;
			uint32				DateFormat() const;

			::Font				Font() const;
			float				FontSize() const;
			rgb_color			Color() const;
			float				GlyphSpacing() const;

 private:
			void				_UpdateTime(bool updateStrings);
			void				_GetCurrentTime();
			void				_GetCurrentDate();

			BoolProperty*		fAnalogFace;
			OptionProperty*		fTimeFormat;
			OptionProperty*		fDateFormat;
			IntProperty*		fDateOffset;
			FontProperty*		fFont;
			FloatProperty*		fFontSize;
			ColorProperty*		fColor;
			FloatProperty*		fGlyphSpacing;

			time_t				fTime;

			BString				fTimeString;
			BString				fDateString;
			bigtime_t			fLastCheck;
};

#endif // CLOCK_CLIP_H

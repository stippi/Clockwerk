/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCROLLING_TEXT_CLIP_H
#define SCROLLING_TEXT_CLIP_H

#include <String.h>

#include "Clip.h"
#include "Font.h"

//class BMessageFilter;
class BoolProperty;
class ColorProperty;
class FloatProperty;
class FontProperty;
class IntProperty;
class StringProperty;

class ScrollingTextClip : public Clip {
 public:
								ScrollingTextClip(
									const char* name = NULL);
								ScrollingTextClip(
									const ScrollingTextClip& other);
	virtual						~ScrollingTextClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	// ScrollingTextClip
			void				SetText(const char* text);

			const char*			Text() const;
			::Font				Font() const;
			float				FontSize() const;
			rgb_color			Color() const;
			bool				UseOutline() const;
			rgb_color			OutlineColor() const;
			float				ScrollingSpeed() const;
			int32				ScrollOffsetResetTimeout() const;
			float				Width() const;

 private:
			void				_UpdateTextWidthAndHeight(BRect canvasBounds);

			StringProperty*		fText;
			FontProperty*		fFont;
			FloatProperty*		fFontSize;
			ColorProperty*		fColor;
			BoolProperty*		fUseOutline;
			ColorProperty*		fOutlineColor;
			FloatProperty*		fScrollingSpeed;
			IntProperty*		fScrollOffsetResetTimeout;
			FloatProperty*		fWidth;

			float				fTextHeight;

//			BMessageFilter*		fFilter;
};

#endif // SCROLLING_TEXT_CLIP_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TEXT_CLIP_H
#define TEXT_CLIP_H

#include <String.h>

#include "Clip.h"
#include "Font.h"

class BMessageFilter;
class BoolProperty;
class ColorProperty;
class FloatProperty;
class FontProperty;
class OptionProperty;
class StringProperty;
class TextBlockRenderer;

class TextClip : public Clip {
 public:
								TextClip(const char* name = NULL);
								TextClip(const TextClip& other);
	virtual						~TextClip();

	// Clip interface
	virtual	status_t			SetTo(const ServerObject* other);

	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	// TextClip
			void				SetText(const char* text);
			void				SetParagraphInset(float inset);
			void				SetBlockWidth(float width);

			const char*			Text() const;
			::Font				Font() const;
			float				FontSize() const;
			rgb_color			Color() const;
			float				ParagraphInset() const;
			float				ParagraphSpacing() const;
			float				LineSpacing() const;
			float				GlyphSpacing() const;
			bool				Hinting() const;
			float				BlockWidth() const;
			uint8				Alignment() const;

			void				GetBaselinePositions(int32 startGlyphIndex,
									int32 endGlyphIndex, BPoint* _startOffset,
									BPoint* _endOffset);
			int32				FirstGlyphIndexAtLine(int32 currentGlyphIndex);
			int32				LastGlyphIndexAtLine(int32 currentGlyphIndex);
			int32				NextGlyphAtLineOffset(int32 currentGlyphIndex,
									int32 lineOffset);
			
 private:
			void				_UpdateCachedLayouter();
			void				_UpdateLayouter(
									TextBlockRenderer* textLayouter);

			StringProperty*		fText;
			FontProperty*		fFont;
			FloatProperty*		fFontSize;
			ColorProperty*		fColor;

			FloatProperty*		fParagraphInset;
			FloatProperty*		fParagraphSpacing;
			FloatProperty*		fLineSpacing;
			FloatProperty*		fGlyphSpacing;
			BoolProperty*		fHinting;
			FloatProperty*		fBlockWidth;
			OptionProperty*		fAlignment;

			TextBlockRenderer*	fTextLayouter;
};

#endif // TEXT_CLIP_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TextClip.h"

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
#include "Icons.h"
#include "MessageConstants.h"
#include "OptionProperty.h"
#include "Property.h"
#include "TextBlockRenderer.h"

using std::nothrow;

// constructor
TextClip::TextClip(const char* name)
	: Clip("TextClip", name)

	, fText(dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_TEXT)))
	, fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT)))
	, fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE)))
	, fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR)))

	, fParagraphInset(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_PARAGRAPH_INSET)))
	, fParagraphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_PARAGRAPH_SPACING)))
	, fLineSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_LINE_SPACING)))
	, fGlyphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_GLYPH_SPACING)))
	, fHinting(dynamic_cast<BoolProperty*>(
			FindProperty(PROPERTY_FONT_HINTING)))
	, fBlockWidth(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_BLOCK_WIDTH)))
	, fAlignment(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_HORIZONTAL_ALIGNMENT)))

	, fTextLayouter(NULL)
{
}

// constructor
TextClip::TextClip(const TextClip& other)
	: Clip(other, true)

	, fText(dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_TEXT)))
	, fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT)))
	, fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE)))
	, fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR)))

	, fParagraphInset(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_PARAGRAPH_INSET)))
	, fParagraphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_PARAGRAPH_SPACING)))
	, fLineSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_LINE_SPACING)))
	, fGlyphSpacing(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_GLYPH_SPACING)))
	, fHinting(dynamic_cast<BoolProperty*>(
			FindProperty(PROPERTY_FONT_HINTING)))
	, fBlockWidth(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_BLOCK_WIDTH)))
	, fAlignment(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_HORIZONTAL_ALIGNMENT)))

	, fTextLayouter(NULL)
{
}

// destructor
TextClip::~TextClip()
{
	delete fTextLayouter;
}

// SetTo
status_t
TextClip::SetTo(const ServerObject* other)
{
	status_t ret = Clip::SetTo(other);
	if (ret == B_OK && fTextLayouter)
		_UpdateCachedLayouter();

	return ret;
}

// Duration
uint64
TextClip::Duration()
{
	// TODO: ...
	return 300;
}

// Bounds
BRect
TextClip::Bounds(BRect canvasBounds)
{
	AffineTransform identityTransform;

	TextBlockRenderer layouter;
	_UpdateLayouter(&layouter);

	return layouter.Bounds(identityTransform);
}

// GetIcon
bool
TextClip::GetIcon(BBitmap* icon)
{
	return GetBuiltInIcon(icon, kTextIcon);
}

// #pragma mark -

// SetText
void
TextClip::SetText(const char* text)
{
	if (!fText)
		return;

	if (fText->SetValue(text))
		ValueChanged(fText);
}

// SetParagraphInset
void
TextClip::SetParagraphInset(float inset)
{
	if (!fParagraphInset)
		return;

	if (fParagraphInset->SetValue(inset))
		ValueChanged(fParagraphInset);
}

// SetBlockWidth
void
TextClip::SetBlockWidth(float width)
{
	if (!fBlockWidth)
		return;

	if (fBlockWidth->SetValue(width))
		ValueChanged(fBlockWidth);
}

// #pragma mark -

// Text
const char*
TextClip::Text() const
{
	if (fText)
		return fText->Value();
	return "";
}

// ::Font
::Font
TextClip::Font() const
{
	if (fFont)
		return fFont->Value();
	return ::Font(*be_bold_font);
}

// FontSize
float
TextClip::FontSize() const
{
	if (fFontSize)
		return fFontSize->Value();
	return kDefaultFontSize;
}

// Color
rgb_color
TextClip::Color() const
{
	if (fColor)
		return fColor->Value();
	return kWhite;
}

// ParagraphInset
float
TextClip::ParagraphInset() const
{
	if (fParagraphInset)
		return fParagraphInset->Value();
	return 0.0;
}

// ParagraphSpacing
float
TextClip::ParagraphSpacing() const
{
	if (fParagraphSpacing)
		return fParagraphSpacing->Value();
	return 1.0;
}

// LineSpacing
float
TextClip::LineSpacing() const
{
	if (fLineSpacing)
		return fLineSpacing->Value();
	return 1.0;
}

// GlyphSpacing
float
TextClip::GlyphSpacing() const
{
	if (fGlyphSpacing)
		return fGlyphSpacing->Value();
	return 1.0;
}

// Hinting
bool
TextClip::Hinting() const
{
	if (fHinting)
		return fHinting->Value();
	return true;
}

// BlockWidth
float
TextClip::BlockWidth() const
{
	if (fBlockWidth)
		return fBlockWidth->Value();
	return 300.0;
}

// Alignment
uint8
TextClip::Alignment() const
{
	if (fAlignment)
		return (uint8)fAlignment->CurrentOptionID();
	return ALIGN_BEGIN;
}

// #pragma mark -

// GetBaselinePositions
void
TextClip::GetBaselinePositions(int32 startGlyphIndex,
	int32 endGlyphIndex, BPoint* _startOffset, BPoint* _endOffset)
{
	// TODO: why?
	_UpdateCachedLayouter();

	if (fTextLayouter) {
		fTextLayouter->GetBaselinePositions(startGlyphIndex, endGlyphIndex,
			_startOffset, _endOffset);
	}
}

// FirstGlyphIndexAtLine
int32
TextClip::FirstGlyphIndexAtLine(int32 currentGlyphIndex)
{
	// TODO: why?
	_UpdateCachedLayouter();

	if (fTextLayouter) {
		return fTextLayouter->FirstGlyphIndexAtLine(currentGlyphIndex);
	}
	return 0;
}

// LastGlyphIndexAtLine
int32
TextClip::LastGlyphIndexAtLine(int32 currentGlyphIndex)
{
	// TODO: why?
	_UpdateCachedLayouter();

	if (fTextLayouter) {
		return fTextLayouter->LastGlyphIndexAtLine(currentGlyphIndex);
	}
	return 0;
}

// NextGlyphAtLineOffset
int32
TextClip::NextGlyphAtLineOffset(int32 currentGlyphIndex, int32 lineOffset)
{
	// TODO: why?
	_UpdateCachedLayouter();

	if (fTextLayouter) {
		return fTextLayouter->NextGlyphAtLineOffset(currentGlyphIndex,
			lineOffset);
	}
	return 0;
}

// #pragma mark -

// _UpdateCachedLayouter
void
TextClip::_UpdateCachedLayouter()
{
	if (!fTextLayouter)
		fTextLayouter = new (nothrow) TextBlockRenderer();

	if (fTextLayouter)
		_UpdateLayouter(fTextLayouter);
}

// _UpdateLayouter
void
TextClip::_UpdateLayouter(TextBlockRenderer* textLayouter)
{
	::Font font = Font();
	font.SetSize(FontSize());

	textLayouter->SetFont(font);

	textLayouter->SetLayout(ParagraphInset(), ParagraphSpacing(),
		LineSpacing(), GlyphSpacing(), BlockWidth(), Alignment());

	textLayouter->SetText(Text());
}

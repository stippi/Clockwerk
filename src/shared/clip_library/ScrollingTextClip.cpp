/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScrollingTextClip.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <Message.h>
//#include <MessageFilter.h>

#include "common_constants.h"
#include "ui_defines.h"

#include "ColorProperty.h"
#include "CommonPropertyIDs.h"
#include "FontProperty.h"
#include "Icons.h"
#include "MessageConstants.h"
#include "Property.h"

using std::nothrow;

//class TextFeedMessageFilter : public BMessageFilter {
// public: 
//							TextFeedMessageFilter(ScrollingTextClip* clip);
//	virtual					~TextFeedMessageFilter();
//
//	virtual	filter_result	Filter(BMessage* message, BHandler** target);
//
// private:
//			ScrollingTextClip*	fClip;
//};
//
//
//TextFeedMessageFilter::TextFeedMessageFilter(ScrollingTextClip* clip)
//	: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
//	  fClip(clip)
//{
//}
//
//
//TextFeedMessageFilter::~TextFeedMessageFilter()
//{
//}
//
//
//filter_result
//TextFeedMessageFilter::Filter(BMessage* message, BHandler** target)
//{
//	if (message->what == MSG_TEXT_FEED) {
//		const char* text;
//		if (message->FindString("text", &text) >= B_OK)
//			fClip->SetText(text);
//		return B_SKIP_MESSAGE;
//	}
//	return B_DISPATCH_MESSAGE;
//}

// #pragma mark -

// constructor
ScrollingTextClip::ScrollingTextClip(const char* name)
	: Clip("ScrollingTextClip", name)

	, fText(dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_TEXT)))
	, fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT)))
	, fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE)))
	, fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR)))
	, fUseOutline(dynamic_cast<BoolProperty*>(
			FindProperty(PROPERTY_USE_OUTLINE)))
	, fOutlineColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_OUTLINE_COLOR)))
	, fScrollingSpeed(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_SCROLLING_SPEED)))
	, fScrollOffsetResetTimeout(dynamic_cast<IntProperty*>(
			FindProperty(PROPERTY_SCROLLING_RESET_TIMEOUT)))
	, fWidth(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_BLOCK_WIDTH)))

//	, fFilter(new TextFeedMessageFilter(this))
{
//	if (be_app->Lock()) {
//		be_app->AddCommonFilter(fFilter);
//		be_app->Unlock();
//	}
}

// constructor
ScrollingTextClip::ScrollingTextClip(const ScrollingTextClip& other)
	: Clip(other, true)

	, fText(dynamic_cast<StringProperty*>(
			FindProperty(PROPERTY_TEXT)))
	, fFont(dynamic_cast<FontProperty*>(
			FindProperty(PROPERTY_FONT)))
	, fFontSize(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_FONT_SIZE)))
	, fColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_COLOR)))
	, fUseOutline(dynamic_cast<BoolProperty*>(
			FindProperty(PROPERTY_USE_OUTLINE)))
	, fOutlineColor(dynamic_cast<ColorProperty*>(
			FindProperty(PROPERTY_OUTLINE_COLOR)))
	, fScrollingSpeed(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_SCROLLING_SPEED)))
	, fScrollOffsetResetTimeout(dynamic_cast<IntProperty*>(
			FindProperty(PROPERTY_SCROLLING_RESET_TIMEOUT)))
	, fWidth(dynamic_cast<FloatProperty*>(
			FindProperty(PROPERTY_BLOCK_WIDTH)))

//	, fFilter(new TextFeedMessageFilter(this))
{
//	if (be_app->Lock()) {
//		be_app->AddCommonFilter(fFilter);
//		be_app->Unlock();
//	}
}

// destructor
ScrollingTextClip::~ScrollingTextClip()
{
//	if (be_app->Lock()) {
//		be_app->RemoveCommonFilter(fFilter);
//		delete fFilter;
//		be_app->Unlock();
//	}
}

// Duration
uint64
ScrollingTextClip::Duration()
{
	return 300;
}

// Bounds
BRect
ScrollingTextClip::Bounds(BRect canvasBounds)
{
	::Font font = ::Font();
	font.SetSize(FontSize());

	font_height fh;
	font.GetHeight(&fh);
	float height = fh.ascent + fh.descent;

	BRect bounds(0.0, 0.0, Width() - 1.0, height - 1.0);
	return bounds;
}

// GetIcon
bool
ScrollingTextClip::GetIcon(BBitmap* icon)
{
	return GetBuiltInIcon(icon, kScrollingTextIcon);
}

// #pragma mark -

// SetText
void
ScrollingTextClip::SetText(const char* text)
{
	if (!fText)
		return;
	// TODO: if the a new pending text arrives
	// while the pending text was already displayed on
	// screen but was still pending, then a display
	// glitch will occur (the pending text will just flip
	// to the new pending text without scrolling into view.)

	if (fText->SetValue(text))
		ValueChanged(fText);
}

// Text
const char*
ScrollingTextClip::Text() const
{
	if (fText)
		return fText->Value();
	return "";
}

// Font
::Font
ScrollingTextClip::Font() const
{
	if (fFont)
		return fFont->Value();
	return ::Font(*be_bold_font);
}

// FontSize
float
ScrollingTextClip::FontSize() const
{
	if (fFontSize)
		return fFontSize->Value();
	return kDefaultFontSize;
}

// Color
rgb_color
ScrollingTextClip::Color() const
{
	if (fColor)
		return fColor->Value();
	return kWhite;
}

// UseOutline
bool
ScrollingTextClip::UseOutline() const
{
	if (fUseOutline)
		return fUseOutline->Value();
	return false;
}

// OutlineColor
rgb_color
ScrollingTextClip::OutlineColor() const
{
	if (fOutlineColor)
		return fOutlineColor->Value();
	return kBlack;
}

// ScrollingSpeed
float
ScrollingTextClip::ScrollingSpeed() const
{
	if (fScrollingSpeed)
		return fScrollingSpeed->Value();
	return kDefaultScrollingSpeed;
}

// ScrollOffsetResetTimeout
int32
ScrollingTextClip::ScrollOffsetResetTimeout() const
{
	if (fScrollOffsetResetTimeout)
		return fScrollOffsetResetTimeout->Value();
	return 25;
}

// Width
float
ScrollingTextClip::Width() const
{
	if (fWidth)
		return fWidth->Value();
	return 684.0;
}


/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "InputSlider.h"

#include <stdio.h>

#include <Message.h>
#include <MessageFilter.h>

#include "NummericalTextView.h"

#define SLIDER_POPUP_WIDTH 13

// constructor
InputSlider::InputSlider(const char* name, const char* label,
						 BMessage* model, BHandler* target,
						 int32 min, int32 max, int32 value,
						 const char* formatString)
	: PopupSlider(name, label, model, target, min, max, value, formatString),
	  fTextView(new NummericalTextView(BRect(0, 0 , 20, 20),
	  								   "input",
	  								   BRect(5, 5, 15, 15),
	  								   B_FOLLOW_NONE,
	  								   B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE))
{
	// prepare fTextView
	fTextView->SetWordWrap(false);
	fTextView->SetViewColor(255, 255, 255, 255);
	fTextView->SetValue(Value());
	AddChild(fTextView);
}

// destructor
InputSlider::~InputSlider()
{
}

// layoutprefs
minimax
InputSlider::layoutprefs()
{
	mpm = PopupSlider::layoutprefs();
	mpm.mini.x += SLIDER_POPUP_WIDTH;
	mpm.maxi.x += SLIDER_POPUP_WIDTH;
	return mpm;
}

// layout
BRect
InputSlider::layout(BRect frame)
{
	PopupSlider::layout(frame);

	frame = SliderFrame();

	frame.OffsetBy(-SLIDER_POPUP_WIDTH, 0);
	frame.InsetBy(2, 2);

	fTextView->MoveTo(frame.LeftTop());
	fTextView->ResizeTo(frame.Width(), frame.Height());

	BRect textRect(fTextView->Bounds());
	textRect.InsetBy(1, 1);
	fTextView->SetTextRect(textRect);

	fTextView->SetAlignment(B_ALIGN_LEFT);

	return Frame();
}

// MouseDown
void
InputSlider::MouseDown(BPoint where)
{
	if (fTextView->Frame().Contains(where))
		return;

	fTextView->MakeFocus(true);

	if (SliderFrame().Contains(where)) {
		SetValue(fTextView->IntValue());
		PopupSlider::MouseDown(where);
	}
}

// SetEnabled
void
InputSlider::SetEnabled(bool enable)
{
	PopupSlider::SetEnabled(enable);

	fTextView->MakeEditable(enable);
	rgb_color textColor = (rgb_color){ 0, 0, 0, 255 };
	if (!enable)
		textColor = tint_color(textColor, B_LIGHTEN_2_TINT);
	fTextView->SetFontAndColor(be_plain_font, B_FONT_ALL, &textColor);
}

// ValueChanged
void
InputSlider::ValueChanged(int32 newValue)
{
	PopupSlider::ValueChanged(newValue);

	// change fTextView's value
	if (LockLooper()) {
		fTextView->SetValue(Value());
		UnlockLooper();
	}
}

// DrawSlider
void
InputSlider::DrawSlider(BRect frame, bool enabled)
{
	frame.left -= SLIDER_POPUP_WIDTH;

	rgb_color background = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightShadow;
	rgb_color midShadow;
	rgb_color darkShadow;
	rgb_color light;
	if (enabled) {
		lightShadow = tint_color(background, B_DARKEN_1_TINT);
		midShadow = tint_color(background, B_DARKEN_2_TINT);
		darkShadow = tint_color(background, B_DARKEN_4_TINT);
		light = tint_color(background, B_LIGHTEN_MAX_TINT);
	} else {
		lightShadow = background;
		midShadow = tint_color(background, B_DARKEN_1_TINT);
		darkShadow = tint_color(background, B_DARKEN_2_TINT);
		light = tint_color(background, B_LIGHTEN_1_TINT);
	}

	// frame around text view
	BRect r(frame);
	BeginLineArray(16);
		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), lightShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), lightShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), light);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), light);

		r = fTextView->Frame().InsetByCopy(-1, -1);

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), darkShadow);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), darkShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), background);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), background);

		r.left = r.right + 1;
		r.right = frame.right - 1;

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top + 1.0), midShadow);
		AddLine(BPoint(r.left, r.top),
				BPoint(r.right, r.top), darkShadow);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), midShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), midShadow);

		r.InsetBy(1, 1);

		AddLine(BPoint(r.left, r.bottom),
				BPoint(r.left, r.top), light);
		AddLine(BPoint(r.left + 1.0, r.top),
				BPoint(r.right, r.top), light);
		AddLine(BPoint(r.right, r.top + 1.0),
				BPoint(r.right, r.bottom), lightShadow);
		AddLine(BPoint(r.right - 1.0, r.bottom),
				BPoint(r.left + 1.0, r.bottom), lightShadow);
	EndLineArray();

	r.InsetBy(1, 1);
	SetLowColor(background);
	FillRect(r, B_SOLID_LOW);

	// popup marker
	BPoint center(floorf((r.left + r.right) / 2.0 + 0.5),
				  floorf((r.top + r.bottom) / 2.0 + 0.5));
	BPoint triangle[3];
	triangle[0] = center + BPoint(-2.5, -0.5);
	triangle[1] = center + BPoint(2.5, -0.5);
	triangle[2] = center + BPoint(0.0, 2.0);

	uint32 flags = Flags();
	SetFlags(flags | B_SUBPIXEL_PRECISE);

	SetHighColor(darkShadow);
	FillTriangle(triangle[0], triangle[1], triangle[2]);

	SetFlags(flags);
}


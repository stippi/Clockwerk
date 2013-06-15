/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ColorValueView.h"

#include <stdio.h>

#include <Locker.h>
#include <Message.h>
#include <String.h>
#include <Window.h>

#include "support_ui.h"

#include "ColorPickerPanel.h"
#include "PropertyItemView.h"
#include "SwatchValueView.h"

enum {
	MSG_VALUE_CHANGED	= 'vchd',
	MSG_SET_COLOR		= 'stcl',
};

// global initializers
ColorPickerPanel*
ColorValueView::sColorPicker = NULL;

BRect
ColorValueView::sColorPickerFrame = BRect(100.0, 100.0, 400.0, 400.0);

selected_color_mode
ColorValueView::sColorPickerMode = H_SELECTED;

BLocker
ColorValueView::sColorPickerSetupLock("color picker setup");

// constructor
ColorValueView::ColorValueView(ColorProperty* property)
	: PropertyEditorView(),
	  fProperty(property)
{
	fSwatchView = new SwatchValueView("swatch property view",
									  new BMessage(MSG_SET_COLOR), this,
									  fProperty->Value());
	fSwatchView->SetDroppedMessage(new BMessage(MSG_VALUE_CHANGED));
	AddChild(fSwatchView);
}

// destructor
ColorValueView::~ColorValueView()
{
}

// Draw
void
ColorValueView::Draw(BRect updateRect)
{
	BRect b(Bounds());
	if (fSwatchView->IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(b);
		b.InsetBy(1.0, 1.0);
		updateRect = updateRect & b;
	}
	FillRect(b, B_SOLID_LOW);
}

// FrameResized
void
ColorValueView::FrameResized(float width, float height)
{
	BRect b(Bounds());
	b.InsetBy(2.0, 2.0);
	b.left = floorf(b.left + (b.Width() / 2.0) - b.Height() / 2.0);
	b.right = b.left + b.Height();
	
	fSwatchView->MoveTo(b.LeftTop());
	fSwatchView->ResizeTo(b.Width(), b.Height());
}

// MakeFocus
void
ColorValueView::MakeFocus(bool focused)
{
	fSwatchView->MakeFocus(focused);
}

// MessageReceived
void
ColorValueView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PASTE:
			fSwatchView->MessageReceived(message);
			break;
		case MSG_SET_COLOR: {

			sColorPickerSetupLock.Lock();

			// if message contains these fields,
			// then it comes from the color picker panel.
			// it also means the panel has died.
			BRect frame;
			selected_color_mode mode;
			if (message->FindRect("panel frame", &frame) == B_OK
				&& message->FindInt32("panel mode", (int32*)&mode) == B_OK) {
				// message came from the color picker panel
				// we remember the settings of the panel for later
				sColorPickerFrame = frame;
				sColorPickerMode = mode;
				// color picker panel has quit
				sColorPicker = NULL;

				rgb_color color;
				if (restore_color_from_message(message, color) == B_OK) {
					if (fProperty->SetValue(color)) {
						fSwatchView->SetColor(color);
						ValueChanged(fProperty);
					}
				}
			} else {
				if (!sColorPicker) {
					sColorPicker = new ColorPickerPanel(
											sColorPickerFrame,
											fProperty->Value(),
											sColorPickerMode,
											Window(),
											new BMessage(MSG_SET_COLOR),
											this);
					sColorPicker->Show();
				} else {
					if (sColorPicker->Lock()) {
						sColorPicker->SetColor(fProperty->Value());
						sColorPicker->SetTarget(this);
						sColorPicker->Activate();
						sColorPicker->Unlock();
					}
				}
			}

			sColorPickerSetupLock.Unlock();

			break;
		}
		case MSG_VALUE_CHANGED: {
			rgb_color c;
			if (restore_color_from_message(message, c) >= B_OK
				&& fProperty->SetValue(c)) {
				ValueChanged(fProperty);
			}
			break;
		}
		default:
			PropertyEditorView::MessageReceived(message);
	}
}

// SetEnabled
void
ColorValueView::SetEnabled(bool enabled)
{
//	if (fEnabled != enabled) {
//		fEnabled = enabled;
//		Invalidate();
//	}
}

// IsFocused
bool
ColorValueView::IsFocused() const
{
	return fSwatchView->IsFocus();
}

// AdoptProperty
bool
ColorValueView::AdoptProperty(Property* property)
{
	ColorProperty* p = dynamic_cast<ColorProperty*>(property);
	if (p) {
		rgb_color ownColor = fProperty->Value();
		rgb_color color = p->Value();
		if (ownColor != color) {
			fSwatchView->SetColor(color);
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
ColorValueView::GetProperty() const
{
	return fProperty;
}




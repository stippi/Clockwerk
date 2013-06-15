/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FontValueView.h"

#include <stdio.h>

#include <Font.h>
#include <MenuItem.h>
#include <Message.h>
#include <PopUpMenu.h>
#include <Region.h>

#include "support_ui.h"

#include "FontManager.h"
#include "FontPopup.h"
#include "FontProperty.h"

enum {
	MSG_FONT_CHANGED = 'opch',
};

// constructor
FontValueView::FontValueView(FontProperty* property)
	: PropertyEditorView(),
	  fProperty(property),
	  fCurrentFont("<no font>"),
	  fEnabled(true)
{
	if (fProperty)
		fProperty->GetFontName(&fCurrentFont);
}

// destructor
FontValueView::~FontValueView()
{
}

// Draw
void
FontValueView::Draw(BRect updateRect)
{
	BRect b(Bounds());
	// focus indication
	if (IsFocus()) {
		SetHighColor(ui_color(B_KEYBOARD_NAVIGATION_COLOR));
		StrokeRect(b);
		b.InsetBy(1.0, 1.0);
		BRegion clipping;
		clipping.Include(b);
		ConstrainClippingRegion(&clipping);
		b.left --;
	}
	// background
	FillRect(b, B_SOLID_LOW);

	rgb_color labelColor = LowColor();
	if (fEnabled)
		labelColor = tint_color(labelColor, B_DARKEN_MAX_TINT);
	else
		labelColor = tint_color(labelColor, B_DISABLED_LABEL_TINT);

	SetHighColor(labelColor);

	b.InsetBy(2.0, 1.0);

	float center = floorf(b.top + b.Height() / 2.0);

	BPoint arrow[3];
	arrow[0] = BPoint(b.left, center - 3.0);
	arrow[1] = BPoint(b.left, center + 3.0);
	arrow[2] = BPoint(b.left + 3.0, center);

	FillPolygon(arrow, 3);

	b.left += 6.0;

	BFont font;
	GetFont(&font);

	font_height fh;
	font.GetHeight(&fh);

	BString truncated(fCurrentFont);
	font.TruncateString(&truncated, B_TRUNCATE_END, b.Width());

	DrawString(truncated.String(),
			   BPoint(b.left, floorf(center + fh.ascent / 2.0)));
}

// FrameResized
void
FontValueView::FrameResized(float width, float height)
{
/*	float radius = ceilf((height - 6.0) / 2.0);
	float centerX = floorf(Bounds().left + width / 2.0);
	float centerY = floorf(Bounds().top + height / 2.0);
	fCheckBoxRect.Set(centerX - radius, centerY - radius,
					  centerX + radius, centerY + radius);*/
}

// MakeFocus
void
FontValueView::MakeFocus(bool focused)
{
	PropertyEditorView::MakeFocus(focused);
	Invalidate();
}

// MessageReceived
void
FontValueView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_FONT_CHANGED:
			if (fProperty) {
				const char* family;
				const char* style;
				if (message->FindString("font family", &family) == B_OK
					&& message->FindString("font style", &style) == B_OK) {

					Font font = fProperty->Value();
					font.SetFamilyAndStyle(family, style);

					if (fProperty->SetValue(font))
						ValueChanged(fProperty);
				}
			}
			break;
		default:
			PropertyEditorView::MessageReceived(message);
			break;
	}
}

// MouseDown
void
FontValueView::MouseDown(BPoint where)
{
	if (BView* parent = Parent())
		parent->MouseDown(ConvertToParent(where));
	
	if (fProperty) {
		font_family family;
		font_style style;
		fProperty->Value().GetFamilyAndStyle(&family, &style);
		
		BPopUpMenu* menu = new BPopUpMenu("font popup", false, false);
		_PolulateMenu(menu, this, family, style);	

		menu->SetTargetForItems(this);
		menu->SetEnabled(fEnabled);
		
		show_popup_menu(menu, where, this);
	}
}

// KeyDown
void
FontValueView::KeyDown(const char* bytes, int32 numBytes)
{
	bool handled = fEnabled;
	if (fEnabled && numBytes > 0) {
		switch (bytes[0]) {
//			case B_LEFT_ARROW:
//			case B_UP_ARROW:
//				fProperty->SetOptionAtOffset(-1);
//				ValueChanged(fProperty);
//				break;
//
//			case B_RIGHT_ARROW:
//			case B_DOWN_ARROW:
//				fProperty->SetOptionAtOffset(1);
//				ValueChanged(fProperty);
//				break;
			default:
				handled = false;
				break;
		}
	}
	if (!handled)
		PropertyEditorView::KeyDown(bytes, numBytes);
}

// SetEnabled
void
FontValueView::SetEnabled(bool enabled)
{
	if (fEnabled != enabled) {
		fEnabled = enabled;
		Invalidate();
	}
}

// ValueChanged
void
FontValueView::ValueChanged(Property* property)
{
	if (fProperty) {
		fProperty->GetFontName(&fCurrentFont);
		BRect b(Bounds());
		b.InsetBy(1.0, 1.0);
		b.left += 5.0;
		Invalidate(b);
	}
	PropertyEditorView::ValueChanged(property);	
}

// AdoptProperty
bool
FontValueView::AdoptProperty(Property* property)
{
	FontProperty* p = dynamic_cast<FontProperty*>(property);
	if (p) {
		BString currentFont;
		p->GetFontName(&currentFont);
		if (currentFont != fCurrentFont) {
			fCurrentFont = currentFont;
			BRect b(Bounds());
			b.InsetBy(1.0, 1.0);
			b.left += 5.0;
			Invalidate(b);
		}
		fProperty = p;
		return true;
	}
	return false;
}

// GetProperty
Property*
FontValueView::GetProperty() const
{
	return fProperty;
}

// #pragma mark -

// PopulateMenu
void
FontValueView::_PolulateMenu(BMenu* menu, BHandler* target,
							 const char* markedFamily,
							 const char* markedStyle)
{
	if (!menu)
		return;

	FontManager* manager = FontManager::Default();

	if (!manager->Lock())
		return;

	BMenu* fontMenu = NULL;

	font_family family;
	font_style style;

	int32 count = manager->CountFontFiles();
	for (int32 i = 0; i < count; i++) {
		if (!manager->GetFontAt(i, family, style))
			break;

		BMessage* message = new BMessage(MSG_FONT_CHANGED);
		message->AddString("font family", family);
		message->AddString("font style", style);

		FontMenuItem* item = new FontMenuItem(style, family, style,
											  message);
		item->SetTarget(target);

		bool markStyle = false;
		if (!fontMenu
			|| (fontMenu->Name()
				&& strcmp(fontMenu->Name(), family) != 0)) {
			// create new entry
			fontMenu = new BMenu(family);
			fontMenu->AddItem(item);
			menu->AddItem(fontMenu);
			// mark the menu if necessary
			if (markedFamily && strcmp(markedFamily, family) == 0) {
				if (BMenuItem* superItem = fontMenu->Superitem())
					superItem->SetMarked(true);
				markStyle = true;
			}
		} else {
			// reuse old entry
			fontMenu->AddItem(item);
		}
		// mark the item if necessary
		if (markStyle && markedStyle && strcmp(markedStyle, style) == 0)
			item->SetMarked(true);
	}
	
	manager->Unlock();
}



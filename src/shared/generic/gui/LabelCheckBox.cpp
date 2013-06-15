/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include "LabelCheckBox.h"

// constructor
LabelCheckBox::LabelCheckBox(const char* label, BMessage* message,
		BHandler* target, bool active)
	:
	BCheckBox(label, label, message),
	fCachedTarget(target)
{
	SetValue(active);
	SetFont(be_bold_font);
}

// destructor
LabelCheckBox::~LabelCheckBox()
{
}

// AttachedToWindow
void
LabelCheckBox::AttachedToWindow()
{
	BCheckBox::AttachedToWindow();
	SetTarget(fCachedTarget);
}

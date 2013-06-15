/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef LABEL_CHECK_BOX_H
#define LABEL_CHECK_BOX_H

#include <CheckBox.h>

class BMessage;
class BHandler;

class LabelCheckBox : public BCheckBox {
public:
								LabelCheckBox(const char* label,
									BMessage* message, BHandler* target,
									bool active);
	virtual						~LabelCheckBox();

	virtual	void				AttachedToWindow();

private:
			BHandler*			fCachedTarget;
};

#endif // LABEL_CHECK_BOX_H

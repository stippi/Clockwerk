/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef BOOL_VALUE_VIEW_H
#define BOOL_VALUE_VIEW_H

#include "Property.h"
#include "PropertyEditorView.h"

class BoolValueView : public PropertyEditorView {
 public:
								BoolValueView(BoolProperty* property);
	virtual						~BoolValueView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
	virtual	void				FrameResized(float width, float height);

	virtual	void				MakeFocus(bool focused);

	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);

	// PropertyEditorView interface
	virtual	void				SetEnabled(bool enabled);

	virtual	bool				AdoptProperty(Property* property);
	virtual	Property*			GetProperty() const;

 private:
			void				_ToggleValue();

			BoolProperty*		fProperty;

			BRect				fCheckBoxRect;
			bool				fEnabled;
};

#endif // BOOL_VALUE_VIEW_H



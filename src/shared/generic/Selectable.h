/*
 * Copyright 2006-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SELECTABLE_H
#define SELECTABLE_H

#include <SupportDefs.h>

class PropertyObject;

class Selectable {
 public:
								Selectable();
	virtual						~Selectable();

	inline	bool				IsSelected() const
									{ return fSelected; }

	virtual	void				SelectedChanged() = 0;

	virtual	PropertyObject*		GetPropertyObject();

 private:
	friend class Selection;
			void				SetSelected(bool selected);

			bool				fSelected;
};

#endif // SELECTABLE_H

/*
 * Copyright 2006-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SELECTION_H
#define SELECTION_H

#include <List.h>

#include "Observable.h"

class Selectable;

class Selection : public Observable {
 public:
								Selection();
	virtual						~Selection();

	// modify selection
			bool				Select(Selectable* object,
									   bool extend = false);
			void				Deselect(Selectable* object);
			void				DeselectAll();

	// query selection
			Selectable*			SelectableAt(int32 index) const;
			Selectable*			SelectableAtFast(int32 index) const;
			int32				CountSelected() const;
			bool				IsEmpty() const;

 private:
			void				_DeselectAllExcept(Selectable* object);

			BList				fSelected;
};

#endif	// SELECTION_H

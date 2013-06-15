/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef OBSERVER_H
#define OBSERVER_H

#include "Referencable.h"

class Observable;

class Observer : public virtual Referencable {
 public:
								Observer();
	virtual						~Observer();

	virtual	void				ObjectChanged(const Observable* object) = 0;
	virtual	void				ObjectDeleted(const Observable* object);
};

#endif // OBSERVER_H

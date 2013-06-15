/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Observer.h"

// constructor
Observer::Observer()
{
}

// destructor
Observer::~Observer()
{
}

// ObjectDeleted
void
Observer::ObjectDeleted(const Observable* object)
{
	// empty
}

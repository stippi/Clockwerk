/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef ASYNC_OBSERVER_H
#define OBSERVERASYNC_OBSERVER_H_H

#include "AbstractLOAdapter.h"
#include "Observer.h"

class AsyncObserver: public Observer, public AbstractLOAdapter {
 public:
	enum {
		MSG_OBJECT_CHANGED		= 'objc',
		MSG_OBJECT_DELETED		= 'objd',
	};
	
								AsyncObserver(BHandler* target);
	virtual						~AsyncObserver();

	virtual	void				ObjectChanged(const Observable* object);
	virtual	void				ObjectDeleted(const Observable* object);
};

#endif // ASYNC_OBSERVER_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "AsyncObserver.h"

#include <Message.h>

// constructor
AsyncObserver::AsyncObserver(BHandler* target)
	: Observer()
	, AbstractLOAdapter(target)
{
}

// destructor
AsyncObserver::~AsyncObserver()
{
}

// ObjectChanged
void
AsyncObserver::ObjectChanged(const Observable* object)
{
	BMessage message(MSG_OBJECT_CHANGED);
	message.AddPointer("observer", (void*)this);
	message.AddPointer("object", (void*)object);
	DeliverMessage(message);
}

// ObjectDeleted
void
AsyncObserver::ObjectDeleted(const Observable* object)
{
	BMessage message(MSG_OBJECT_DELETED);
	message.AddPointer("observer", (void*)this);
	message.AddPointer("object", (void*)object);
	DeliverMessage(message);
}


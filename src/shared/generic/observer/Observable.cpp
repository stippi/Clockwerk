/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "Observable.h"

#include <stdio.h>
#include <string.h>
#include <typeinfo>

#include <Autolock.h>

#include "Observer.h"

BLocker
Observable::sObserverListLock("observer list");

// constructor
Observable::Observable()
	: fObservers(4),
	  fSuspended(0),
	  fPendingNotifications(false),
	  fDebugNotify(false)
{
}

// destructor
Observable::~Observable()
{
	_NotifyDeleted();

	if (fObservers.CountItems() > 0) {
		PrintObservers();
		char message[256];
		sprintf(message, "Observable::~Observable() - %ld "
						 "observers still watching!\n", fObservers.CountItems());
		debugger(message);
	}
}

// AddObserver
bool
Observable::AddObserver(Observer* observer)
{
	BAutolock _(sObserverListLock);

	if (observer && !fObservers.HasItem((void*)observer)) {
//printf("%p->AddObserver(%p/%s)\n", this, observer, typeid(*observer).name());
		return fObservers.AddItem((void*)observer);
	}
	return false;
}

// RemoveObserver
bool
Observable::RemoveObserver(Observer* observer)
{
	BAutolock _(sObserverListLock);

//printf("%p->RemoveObserver(%p/%s)\n", this, observer, typeid(*observer).name());
	return fObservers.RemoveItem((void*)observer);
}

// Notify
void
Observable::Notify() const
{
	if (!fSuspended) {
		BList observers;
		_AcquireObservers(observers);

		int32 count = observers.CountItems();
		if (fDebugNotify && count > 0) {
			printf("%ld observers\n", count);
		}
		for (int32 i = 0; i < count; i++) {
			Observer* observer = (Observer*)observers.ItemAtFast(i);
			if (fDebugNotify) {
				printf("  %p: %p/%s->Notify()\n", this, observer,
					   typeid(*observer).name());
			}
			observer->ObjectChanged(this);
			observer->Release();
		}
		fPendingNotifications = false;
	} else {
		fPendingNotifications = true;
	}
}

// SuspendNotifications
void
Observable::SuspendNotifications(bool suspend)
{
	if (suspend)
		fSuspended++;
	else
		fSuspended--;

	if (fSuspended < 0) {
		fprintf(stderr, "Observable::SuspendNotifications(false) - "
						"error: suspend level below zero!\n");
		fSuspended = 0;
	}

	if (!fSuspended && fPendingNotifications)
		Notify();
}

// SetDebugNotify
void
Observable::SetDebugNotify(bool debug)
{
	fDebugNotify = debug;
}

// PrintObservers
void
Observable::PrintObservers() const
{
	int32 count = fObservers.CountItems();
	printf("%p - %ld observers\n", this, count);

	for (int32 i = 0; i < count; i++) {
		Observer* observer = (Observer*)fObservers.ItemAtFast(i);
		printf("  %p/%s\n", observer, typeid(*observer).name());
	}
}

// #pragma mark -

// _AcquireObservers
void
Observable::_AcquireObservers(BList& observers) const
{
	// clone observer list and acquire a reference to each observer
	BAutolock _(sObserverListLock);

	observers.AddList(const_cast<BList*>(&fObservers));
	int32 count = observers.CountItems();
	for (int32 i = 0; i < count; i++)
		((Observer*)observers.ItemAtFast(i))->Acquire();
}

// _NotifyDeleted
void
Observable::_NotifyDeleted() const
{
	BList observers;
	_AcquireObservers(observers);

	int32 count = observers.CountItems();
	for (int32 i = 0; i < count; i++) {
		Observer* observer = (Observer*)observers.ItemAtFast(i);
		observer->ObjectDeleted(this);
		observer->Release();
	}
}


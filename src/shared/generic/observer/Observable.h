/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef OBSERVABLE_H
#define OBSERVABLE_H

#include <List.h>
#include <Locker.h>

class Observer;

class Observable {
 public:
								Observable();
	virtual						~Observable();

			bool				AddObserver(Observer* observer);
			bool				RemoveObserver(Observer* observer);

	 		void				Notify() const;

			void				SuspendNotifications(bool suspend);

			void				SetDebugNotify(bool debug);
			void				PrintObservers() const;

 private:
	 		void				_AcquireObservers(BList& observers) const;
	 		void				_NotifyDeleted() const;

 private:
			BList				fObservers;

			int32				fSuspended;
	mutable	bool				fPendingNotifications;

			bool				fDebugNotify;

	static	BLocker				sObserverListLock;
		// TODO: change this to a lock that has one
		// semaphore per thread and is not static
};

class AutoNotificationSuspender {
 public:
								AutoNotificationSuspender(Observable* object)
									: fObject(object)
								{
									fObject->SuspendNotifications(true);
								}

	virtual						~AutoNotificationSuspender()
								{
									fObject->SuspendNotifications(false);
								}
 private:
			Observable*			fObject;
};

#endif // OBSERVABLE_H

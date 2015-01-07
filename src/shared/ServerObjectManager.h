/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SERVER_OBJECT_MANAGER_H
#define SERVER_OBJECT_MANAGER_H

#include <List.h>
#include <String.h>

#include "AbstractLOAdapter.h"
#include "HashMap.h"
#include "HashString.h"
#include "RWLocker.h"


class ProgressReporter;
class ServerObject;
class ServerObjectFactory;
struct entry_ref;

class SOMListener {
 public:
								SOMListener();
	virtual						~SOMListener();

	virtual	void				ObjectAdded(ServerObject* object,
											int32 index) = 0;
	virtual	void				ObjectRemoved(ServerObject* object) = 0;
};

class AsyncSOMListener : public SOMListener, public AbstractLOAdapter {
 public:
	enum {
		MSG_OBJECT_ADDED		= 'obja',
		MSG_OBJECT_REMOVED		= 'objr',
	};

								AsyncSOMListener(BHandler* handler);
	virtual						~AsyncSOMListener();

	virtual	void				ObjectAdded(ServerObject* object, int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);
};

class ServerObjectManager {
 public:
								ServerObjectManager();
	virtual						~ServerObjectManager();

			void				SetLocker(RWLocker* locker);
			RWLocker*			Locker() const
									{ return fLocker; }

			bool				ReadLock();
			status_t			ReadLockWithTimeout(bigtime_t timeout);
			void				ReadUnlock();
			bool				IsReadLocked() const;

			bool				WriteLock();
			status_t			WriteLockWithTimeout(bigtime_t timeout);
			void				WriteUnlock();
			bool				IsWriteLocked() const;

	// list operations
			bool				AddObject(ServerObject* object);
			bool				AddObject(ServerObject* object, int32 index);
			bool				RemoveObject(ServerObject* object);
			ServerObject*		RemoveObject(int32 index);

			int32				CountObjects() const;
			bool				HasObject(ServerObject* object) const;
			int32				IndexOf(ServerObject* object) const;

			void				MakeEmpty();

			ServerObject*		ObjectAt(int32 index) const;
			ServerObject*		ObjectAtFast(int32 index) const;

	// misc
			ServerObject*		FindObject(const BString& id) const;
			status_t			IDChanged(ServerObject* object,
									const BString& oldID);

			status_t			ResolveDependencies(
									ProgressReporter* reporter = NULL) const;

			void				UpdateVersion(const BString& id,
									int32 version);

	static	void				SetClientID(const char* clientID);
	static	BString				NextID();

	// state saving
			void				ForceStateChanged();
	virtual	void				StateChanged();
	virtual void				SetIgnoreStateChanges(bool ignore);
			status_t			FlushObjects(ServerObjectFactory* factory);
	virtual	bool				IsStateSaved() const;

 protected:
			void				StateSaved();

 public:
	// behavior flags
			void				SetLoadRemovedObjects(bool load);
			bool				LoadRemovedObjects() const
									{ return fLoadRemovedObjects; }

			bool				AddListener(SOMListener* listener);
			bool				RemoveListener(SOMListener* listener);

	virtual	void				SetDirectory(const char* directory);
	virtual	const char*			Directory() const
									{ return fDirectory.String(); }
	virtual	status_t			GetRef(const BString& id, entry_ref& ref);
	virtual status_t			GetRef(const ServerObject* object, entry_ref& ref);

 private:
			void				_StateChanged();
			void				_MakeEmpty();

			void				_NotifyObjectAdded(
									ServerObject* object, int32 index) const;
			void				_NotifyObjectRemoved(
									ServerObject* object) const;

			RWLocker*			fLocker;

	typedef HashMap<HashString, HashKey32<ServerObject*> > IDObjectMap;

			BString				fDirectory;

			BList				fObjects;
			IDObjectMap			fIDObjectMap;

			BList				fListeners;

	static	int64				sBaseID;
	static	int32				sNextID;
	static	BString				sClientID;

			bool				fStateNeedsSaving;
			bool				fLoadRemovedObjects;
};

#endif // SERVER_OBJECT_MANAGER_H

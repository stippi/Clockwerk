/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ID_MANAGER_H
#define ID_MANAGER_H

#include <List.h>
#include <String.h>

class IDManager
{
 public:
								IDManager(int32 initialCapacity = 100);
	virtual						~IDManager();

			void				AssociateObjectWithID(const void* object,
													  const char* id);
			const char*			GetIDForObject(const void* object);
			void*				GetObjectForID(const char* id) const;
			bool				RemoveObject(const void* object);
			bool				RemoveID(const char* id);

 private:
	struct Entry
	{
			BString				id;
			void*				object;
	};

 private:
			Entry*				_AssociateObjectWithID(const void* object,
													   const char* id);
			void				_RemoveEntry(const void* object,
											 const char* id);
			int32				_CountEntries() const;
			Entry*				_IDEntryAt(int32 index) const;
			Entry*				_ObjectEntryAt(int32 index) const;
			int32				_FindObjectInsertionIndex(
									const void* object) const;
			int32				_FindIDInsertionIndex(const char* id) const;

			void				_GenerateNewID(BString& id);

 private:
			BList				fObjectToID;
			BList				fIDToObject;
			int32				fNextID;
};

#endif	// ID_MANAGER_H

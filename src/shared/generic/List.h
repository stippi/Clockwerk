/*
 * Copyright 2000-2003, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef LIST_H
#define LIST_H

#include <List.h>

template<class T, bool delete_on_destruction = true>
class List : protected BList {
 public:
	List(int32 count = 10)
		: BList(count) {}

	~List()
		{ MakeEmpty(); }

	// adding items
	inline bool AddItem(T value)
		{ return BList::AddItem((void*)value); }

	inline bool AddItem(T value, int32 index)
		{ return BList::AddItem((void*)value, index); }

	inline bool AddList(List<T, false>* list)
		{ return BList::AddList(list); }
	inline bool MoveFrom(List<T, true>* list)
		{
			if (BList::AddList(list)) {
				BList::MakeEmpty(list);
				return true;
			}
			return false;
		}

	// information
	inline bool HasItem(T value) const
		{ return BList::HasItem((void*)value); }

	inline int32 IndexOf(T value) const
		{ return BList::IndexOf((void*)value); }

	inline bool IsEmpty() const
		{ return BList::IsEmpty(); }

	inline int32 CountItems() const
		{ return BList::CountItems(); }

	// retrieving items
	inline T ItemAt(int32 index) const
		{ return (T)BList::ItemAt(index); }

	inline T ItemAtFast(int32 index) const
		{ return (T)BList::ItemAtFast(index); }

	inline T FirstItem() const
		{ return (T)BList::FirstItem(); }

	inline T LastItem() const
		{ return (T)BList::LastItem(); }

	inline T* Items() const
		{ return (T*)BList::Items(); }

	// removing items
	inline bool RemoveItem(T value)
		{ return BList::RemoveItem((void*)value); }

	inline T RemoveItem(int32 index)
		{ return (T)BList::RemoveItem(index); }

	inline T RemoveItemAt(int32 index)
		{ return (T)BList::RemoveItem(index); }

	inline bool RemoveItems(int32 index, int32 count)
		{ return BList::RemoveItems(index, count); }

	inline void MakeEmpty() {
		if (delete_on_destruction) {
			// delete all values
			int32 count = CountItems();
			for (int32 i = 0; i < count; i++)
				delete (T)BList::ItemAtFast(i);
		}
		BList::MakeEmpty();
	}

	inline void SortItems(int (*func)(const void* a, const void* b))
		{ return BList::SortItems(func); }
};

#endif	// LIST_H

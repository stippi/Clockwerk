/*
 * Copyright 2001-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Ingo Weinhold
 */
#ifndef REFERENCABLE_H
#define REFERENCABLE_H

#include <stdio.h>

#include <SupportDefs.h>

class Referencable {
 public:
								Referencable();
	virtual						~Referencable();

	inline	void				Acquire();
	inline	bool				Release();

			void				SetDebugRelease(bool debug);
	static	int32				DebuggedReferencableCount();

 private:
			vint32				fReferenceCount;
			bool				fDebug;
	static	vint32				sDebuggedReferencableCount;
};

// Acquire
inline void
Referencable::Acquire()
{
	atomic_add(&fReferenceCount, 1);
}

// Release
inline bool
Referencable::Release()
{
	int32 previousRefCount = atomic_add(&fReferenceCount, -1);
	if (fDebug)
		printf("Referencable::Release() - %ld\n", previousRefCount);
	if (previousRefCount == 1) {
		if (fDebug)
			atomic_add(&sDebuggedReferencableCount, -1);
		delete this;
		return true;
	}

	return false;
}

// #pragma mark -

class Reference {
 public:
	Reference()
		: fObject(NULL)
	{
	}

	Reference(Referencable* object, bool alreadyHasReference = false)
		: fObject(NULL)
	{
		SetTo(object, alreadyHasReference);
	}

	Reference(const Reference& other)
		: fObject(NULL)
	{
		SetTo(other.fObject);
	}

	~Reference()
	{
		Unset();
	}

	void SetTo(Referencable* object, bool alreadyHasReference = false)
	{
		Unset();
		fObject = object;
		if (fObject && !alreadyHasReference)
			fObject->Acquire();
	}

	void Unset()
	{
		if (fObject) {
			fObject->Release();
			fObject = NULL;
		}
	}

	Referencable* Detach()
	{
		Referencable* object = fObject;
		fObject = NULL;
		return object;
	}
 private:
	Referencable*	fObject;
};


#endif // REFERENCABLE_H

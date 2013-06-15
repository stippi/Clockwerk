/*
 * Copyright 2001-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Axel Dörfler, axeld@pinc-software.de
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#include "Referencable.h"


vint32
Referencable::sDebuggedReferencableCount = 0;


// constructor
Referencable::Referencable()
	: fReferenceCount(1)
	, fDebug(false)
{
}

// destructor
Referencable::~Referencable()
{
}

// SetDebugRelease
void
Referencable::SetDebugRelease(bool debug)
{
	fDebug = debug;

	if (fDebug)
		atomic_add(&sDebuggedReferencableCount, 1);
	else 
		atomic_add(&sDebuggedReferencableCount, -1);
}

// DebuggedReferencableCount
/*static*/ int32
Referencable::DebuggedReferencableCount()
{
	return sDebuggedReferencableCount;
}


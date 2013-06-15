/*
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "VCTarget.h"

// constructor
VCTarget::VCTarget()
	: fBitmapLock(),
	  fBitmap(NULL)
{
}

// destructor
VCTarget::~VCTarget()
{
}

// LockBitmap
bool
VCTarget::LockBitmap()
{
	return fBitmapLock.Lock();
}

// UnlockBitmap
void
VCTarget::UnlockBitmap()
{
	fBitmapLock.Unlock();
}

// SetBitmap
void
VCTarget::SetBitmap(const BBitmap* bitmap)
{
	LockBitmap();
	fBitmap = bitmap;
	UnlockBitmap();
}

// GetBitmap
const BBitmap*
VCTarget::GetBitmap() const
{
	return fBitmap;
}


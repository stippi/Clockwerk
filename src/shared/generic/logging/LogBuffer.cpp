/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "LogBuffer.h"

#include <new>

#include <string.h>


using std::nothrow;


// destructor
LogBuffer::LogBuffer()
	: fCapacity(0),
	  fBuffer(NULL),
	  fSize(0)
{
}


// destructor
LogBuffer::~LogBuffer()
{
	delete[] fBuffer;
}


// Init
status_t
LogBuffer::Init(size_t capacity)
{
	fBuffer = new(nothrow) char[capacity];
	if (fBuffer == NULL)
		return B_NO_MEMORY;

	fCapacity = capacity;

	return B_OK;
}


// Append
bool
LogBuffer::Append(const char* text, size_t size)
{
	if (fSize + size > fCapacity)
		return false;

	memcpy(fBuffer + fSize, text, size);
	fSize += size;

	return true;
}


// Clear
void
LogBuffer::Clear()
{
	fSize = 0;
}



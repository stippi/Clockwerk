/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOG_BUFFER_H
#define LOG_BUFFER_H

#include <SupportDefs.h>


class LogBuffer {
public:
								LogBuffer();
								~LogBuffer();

			status_t			Init(size_t capacity);

			size_t				Capacity() const	{ return fCapacity; }

			char*				Buffer() const		{ return fBuffer; }
			size_t				Size() const		{ return fSize; }

			bool				Append(const char* text, size_t size);
			void				Clear();

private:
			size_t				fCapacity;
			char*				fBuffer;
			size_t				fSize;
};


#endif	// LOG_BUFFER_H

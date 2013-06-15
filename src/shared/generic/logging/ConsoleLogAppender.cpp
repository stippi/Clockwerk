/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ConsoleLogAppender.h"

#include "Logging.h"

#include <stdio.h>


// constructor
ConsoleLogAppender::ConsoleLogAppender(const char* name)
	: LogAppender(name)
{
}


// destructor
ConsoleLogAppender::~ConsoleLogAppender()
{
}


// PutText
void
ConsoleLogAppender::PutText(const char* text, size_t len, int logLevel)
{
	FILE* file = (logLevel <= LOG_LEVEL_ERROR ? stderr : stdout);
	fwrite(text, 1, len, file);
}

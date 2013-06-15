/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CONSOLE_LOG_APPENDER_H
#define CONSOLE_LOG_APPENDER_H

#include "LogAppender.h"


class ConsoleLogAppender : public LogAppender {
public:
								ConsoleLogAppender(const char* name);
	virtual						~ConsoleLogAppender();

protected:
	virtual	void				PutText(const char* text, size_t len,
									int logLevel);
};


#endif	// CONSOLE_LOG_APPENDER_H

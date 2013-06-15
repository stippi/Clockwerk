/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOG_MESSAGE_INFO_H
#define LOG_MESSAGE_INFO_H

#include <sys/time.h>
#include <time.h>

#include <OS.h>


class LogMessageInfo {
public:
								LogMessageInfo(const char* name, int logLevel);

	inline	const char*			Name()			{ return fName; }
	inline	int					LogLevel()		{ return fLogLevel; }
	inline	thread_id			Thread() 		{ return fThreadInfo.thread; }
			const char*			ThreadName();
	inline	const struct tm*	Time()			{ return &fTime; }
	inline	int32				TimeMicros() 	{ return fTimeMicros; }

private:
			const char*			fName;
			int					fLogLevel;
			thread_info			fThreadInfo;
			struct tm			fTime;
			int32				fTimeMicros;
};


#endif	// LOG_MESSAGE_INFO_H

/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "LogMessageInfo.h"


// constructor
LogMessageInfo::LogMessageInfo(const char* name, int logLevel)
	: fName(name),
	  fLogLevel(logLevel)
{
	fThreadInfo.thread = find_thread(NULL);
	fThreadInfo.team = -1;	// indicates that the info is not valid yet

	struct timeval time;
	gettimeofday(&time, NULL);

	time_t timeInSecs = time.tv_sec;
	fTimeMicros = time.tv_usec;

	localtime_r(&timeInSecs, &fTime);
}

// ThreadName
const char*
LogMessageInfo::ThreadName()
{
	if (fThreadInfo.team < 0)
		get_thread_info(fThreadInfo.thread, &fThreadInfo);

	return fThreadInfo.name;
}


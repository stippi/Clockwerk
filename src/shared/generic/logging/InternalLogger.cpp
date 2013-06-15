/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "InternalLogger.h"

#include <stdio.h>

#include "LogAppender.h"
#include "Logging.h"
#include "LogMessageInfo.h"

#include "AutoLocker.h"


// constructor
InternalLogger::InternalLogger(const char* name)
	: fName(name),
	  fThreshold(LOG_LEVEL_NONE),
	  fEffectiveThreshold(LOG_LEVEL_NONE),
	  fAppenders()
{
}


// constructor
InternalLogger::InternalLogger(const char* name, int threshold,
		const BList& appenders)
	: fName(name),
	  fThreshold(threshold),
	  fEffectiveThreshold(LOG_LEVEL_NONE),
	  fAppenders()
{
	SetTo(threshold, appenders);
}


// destructor
InternalLogger::~InternalLogger()
{
	_Unset();
}


// SetTo
void
InternalLogger::SetTo(int threshold, const BList& appenders)
{
	_Unset();

	fEffectiveThreshold = LOG_LEVEL_NONE;
	fThreshold = threshold;

	// grab references to the appenders and compute the effective threshold
	int32 count = appenders.CountItems();
	for (int32 i = 0; i < count; i++) {
		LogAppender* appender = (LogAppender*)appenders.ItemAt(i);
		if (fAppenders.AddItem(appender)) {
			appender->AddReference();

			if (appender->Threshold() > fEffectiveThreshold)
				fEffectiveThreshold = appender->Threshold();
		}
	}

	if (fEffectiveThreshold > fThreshold)
		fEffectiveThreshold = fThreshold;
}


// Name
const char*
InternalLogger::Name() const
{
	return fName;
}


// Log
void
InternalLogger::Log(int level, const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(level, format, args);

	va_end(args);
}


// LogV
void
InternalLogger::LogV(int level, const char* format, va_list args,
	bool formatted)
{
	if (this == NULL)
		return;

	// reference appenders
	AutoLocker<Logging> locker(Logging::Default());

	if (level > fThreshold || fAppenders.IsEmpty())
		return;

	BList appenders;
	int32 count = fAppenders.CountItems();
	for (int32 i = 0; i < count; i++) {
		LogAppender* appender = (LogAppender*)fAppenders.ItemAt(i);
		if (appenders.AddItem(appender))
			appender->AddReference();
	}
	
	locker.Unlock();

	// format message
	const char* buffer;
	char stackBuffer[1024];
	if (formatted) {
		vsnprintf(stackBuffer, sizeof(stackBuffer), format, args);
		buffer = stackBuffer;
	} else
		buffer = format;

	LogMessageInfo info(fName, level);

	// append
	count = appenders.CountItems();
	for (int32 i = 0; i < count; i++) {
		LogAppender* appender = (LogAppender*)appenders.ItemAt(i);
		appender->AppendMessage(info, buffer);
		appender->RemoveReference();
	}
}


// _Unset
void
InternalLogger::_Unset()
{
	// dereference the appenders and clear the list
	int32 count = fAppenders.CountItems();
	for (int32 i = 0; i < count; i++) {
		LogAppender* appender = (LogAppender*)fAppenders.ItemAt(i);
		appender->RemoveReference();
	}

	fAppenders.MakeEmpty();
}


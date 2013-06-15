/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Logger.h"

#include "InternalLogger.h"


// constructor
Logger::Logger(const char* name)
	: fName(name),
	  fLogger(NULL)
{
}


// _Logger
inline InternalLogger*
Logger::_Logger() const
{
	if (fLogger == NULL)
		fLogger = Logging::Default()->LoggerFor(fName);

	return fLogger;
}


// Threshold
int
Logger::Threshold() const
{
	return _Logger()->Threshold();
}


// EffectiveThreshold
int
Logger::EffectiveThreshold() const
{
	return _Logger()->EffectiveThreshold();
}


// LogV
void
Logger::LogV(int level, const char* format, va_list args)
{
	_Logger()->LogV(level, format, args, true);
}


// LogUnformatted
void
Logger::LogUnformatted(int level, const char* format)
{
	va_list args;
	memset(&args, 0, sizeof(va_list));
	_Logger()->LogV(level, format, args, false);
}


// Log
void
Logger::Log(int level, const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(level, format, args);

	va_end(args);
}


// Fatal
void
Logger::Fatal(const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(LOG_LEVEL_FATAL, format, args);

	va_end(args);
}


// Error
void
Logger::Error(const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(LOG_LEVEL_ERROR, format, args);

	va_end(args);
}


// Warn
void
Logger::Warn(const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(LOG_LEVEL_WARN, format, args);

	va_end(args);
}


// Info
void
Logger::Info(const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(LOG_LEVEL_INFO, format, args);

	va_end(args);
}


// Debug
void
Logger::Debug(const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(LOG_LEVEL_DEBUG, format, args);

	va_end(args);
}


// Trace
void
Logger::Trace(const char* format,...)
{
	va_list args;
	va_start(args, format);

	LogV(LOG_LEVEL_TRACE, format, args);

	va_end(args);
}

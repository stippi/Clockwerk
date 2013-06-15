/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdarg.h>

#include "Logging.h"


class InternalLogger;


// log macros
#define LOG(logger, level, format...)		\
	if ((logger).IsLogLevelEnabled(level))	\
		(logger).Log((level), format)
#define LOG_FATAL(format...)	LOG(sLog, LOG_LEVEL_FATAL, format)
#define LOG_ERROR(format...)	LOG(sLog, LOG_LEVEL_ERROR, format)
#define LOG_WARN(format...)		LOG(sLog, LOG_LEVEL_WARN, format)
#define LOG_INFO(format...)		LOG(sLog, LOG_LEVEL_INFO, format)
#define LOG_DEBUG(format...)	LOG(sLog, LOG_LEVEL_DEBUG, format)
#define LOG_TRACE(format...)	LOG(sLog, LOG_LEVEL_TRACE, format)


class Logger {
public:
								Logger(const char* name);

			int					Threshold() const;
			int					EffectiveThreshold() const;

	inline	bool				IsLogLevelEnabled(int level) const;

	inline	bool				IsFatalEnabled() const;
	inline	bool				IsErrorEnabled() const;
	inline	bool				IsWarnEnabled() const;
	inline	bool				IsInfoEnabled() const;
	inline	bool				IsDebugEnabled() const;
	inline	bool				IsTraceEnabled() const;

			void				LogV(int level, const char* format,
									va_list args);
			void				Log(int level, const char* format,...)
									PRINTF_LIKE(3, 4);
			void				LogUnformatted(int level, const char* message);
			void				Fatal(const char* format,...) PRINTF_LIKE(2, 3);
			void				Error(const char* format,...) PRINTF_LIKE(2, 3);
			void				Warn(const char* format,...) PRINTF_LIKE(2, 3);
			void				Info(const char* format,...) PRINTF_LIKE(2, 3);
			void				Debug(const char* format,...) PRINTF_LIKE(2, 3);
			void				Trace(const char* format,...) PRINTF_LIKE(2, 3);

private:
	inline	InternalLogger*		_Logger() const;

private:
			const char*			fName;
	mutable	InternalLogger*		fLogger;
};


// IsLogLevelEnabled
inline bool
Logger::IsLogLevelEnabled(int level) const
{
	return (level <= EffectiveThreshold());
}


// IsFatalEnabled
inline bool
Logger::IsFatalEnabled() const
{
	return IsLogLevelEnabled(LOG_LEVEL_FATAL);
}


// IsErrorEnabled
inline bool
Logger::IsErrorEnabled() const
{
	return IsLogLevelEnabled(LOG_LEVEL_ERROR);
}


// IsWarnEnabled
inline bool
Logger::IsWarnEnabled() const
{
	return IsLogLevelEnabled(LOG_LEVEL_WARN);
}


// IsInfoEnabled
inline bool
Logger::IsInfoEnabled() const
{
	return IsLogLevelEnabled(LOG_LEVEL_INFO);
}


// IsDebugEnabled
inline bool
Logger::IsDebugEnabled() const
{
	return IsLogLevelEnabled(LOG_LEVEL_DEBUG);
}


// IsTraceEnabled
inline bool
Logger::IsTraceEnabled() const
{
	return IsLogLevelEnabled(LOG_LEVEL_TRACE);
}


#endif	// LOGGER_H

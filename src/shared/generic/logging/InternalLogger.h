/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INTERNAL_LOGGER_H
#define INTERNAL_LOGGER_H

#include <stdarg.h>

#include <List.h>

#include "Logging.h"


class InternalLogger {
public:
								InternalLogger(const char* name);
								InternalLogger(const char* name, int threshold,
									const BList& appenders);
								~InternalLogger();

			void				SetTo(int threshold, const BList& appenders);
									// Logging lock must be held

			const char*			Name() const;

	inline	int					Threshold() const;
	inline	int					EffectiveThreshold() const;

			void				Log(int level, const char* format,...)
									 PRINTF_LIKE(3, 4);
			void				LogV(int level, const char* format,
									va_list args, bool formatted = true);

private:
			void				_Unset();

private:
			const char*			fName;
			int					fThreshold;
			int					fEffectiveThreshold;
			BList				fAppenders;
};


// Threshold
inline int
InternalLogger::Threshold() const
{
	if (this == NULL)
		return LOG_LEVEL_NONE;

	return fThreshold;
}


// EffectiveThreshold
inline int
InternalLogger::EffectiveThreshold() const
{
	if (this == NULL)
		return LOG_LEVEL_NONE;

	return fEffectiveThreshold;
}


#endif	// INTERNAL_LOGGER_H

/*
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COMMON_LOGGING_H
#define COMMON_LOGGING_H

#include <SupportDefs.h>


status_t init_logging(const char* appName, const char* logSettingsPath,
	bool watchLogSettings);

#endif // COMMON_LOGGING_H

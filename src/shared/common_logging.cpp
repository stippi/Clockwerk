/*
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "common_logging.h"

#include <stdio.h>

#include <new>

#include <Looper.h>

#include "Logger.h"
#include "Logging.h"
#include "svn_revision.h"


// init_logging
status_t
init_logging(const char* appName, const char* logSettingsPath,
	bool watchLogSettings)
{
	// load log settings
	status_t error = Logging::Default()->Init(logSettingsPath);
	if (error != B_OK) {
		fprintf(stderr, "%s: Failed to load log settings! "
			"Logging disabled!\n", appName);
		return error;
	}

	Logger sLog("init_logging");

	// logging initialized
	LOG_FATAL("%s starting, revision %ld...\n", appName, kSVNRevision);
	LOG_INFO("using log.properties: %s\n", logSettingsPath);

	if (!watchLogSettings)
		return B_OK;

	// Start watching the logging config file.
	// Note, we create a separate looper in case the app is never run.
	// We leak (never destroy) the looper, but that doesn't really matter
	// anyway, since logging shall usually be enabled until the app quits.
	BLooper* loggingConfigLooper = new(std::nothrow) BLooper(
		"log config watching");
	if (loggingConfigLooper) {
		loggingConfigLooper->Run();
		if (loggingConfigLooper->Lock()) {
			status_t error = Logging::Default()
				->StartWatchingConfigFile(loggingConfigLooper);
			if (error != B_OK) {
				LOG_ERROR("Starting watching config file failed: %s\n",
					strerror(error));
			}
			loggingConfigLooper->Unlock();
		}
	} else {
		LOG_ERROR("Insufficient memory while creating logging config "
			"watching looper\n");
	}

	return B_OK;
}

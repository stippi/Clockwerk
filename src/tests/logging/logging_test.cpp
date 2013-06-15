/*
 * Copyright 2006-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Looper.h>

#include "Logger.h"
#include "Logging.h"
#include "JavaProperties.h"
#include "StringTokenizer.h"


static Logger sLog("main");


class A {
public:
	void LogSomething(int32 var)
	{
		LOG_ERROR("%p->A::LogSomething(): %ld\n", this, var);
		LOG_INFO("%p->A::LogSomething(): %ld\n", this, var);
		LOG_DEBUG("%p->A::LogSomething(): %ld\n", this, var);
		LOG_TRACE("%p->A::LogSomething(): %ld\n", this, var);
	}

private:
	static	Logger	sLog;
};

Logger A::sLog("A");


static void
print_threshold(const char* category)
{
	Logger logger(category);
	printf("threshold for \"%s\": %d\n", category, logger.Threshold());
}


int
main()
{
	status_t error = Logging::Default()->Init("/boot/home/develop/mindwork/Clockwerk/src/logging_test/log.properties");
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to init logging: %s\n", strerror(error));
		exit(1);
	}

	LOG_FATAL("logging_test started!\n");

	LOG_INFO("Log message without newline");
	LOG_INFO("Log message with single newline\n");
	LOG_INFO("Log message with two newlines\n\n");

//	A a;
//	for (int32 i = 0; i < 10000; i++)
//		a.LogSomething(i);

	print_threshold("test");
	print_threshold("test.categoryA");
	print_threshold("test.categoryA.1");
	print_threshold("test.categoryA.2");
	print_threshold("test.categoryA.3");
	print_threshold("test.categoryB.1");
	print_threshold("testX.categoryA.1");
	print_threshold("testX.categoryA.2");

	BLooper* looper = new BLooper("test looper");
	looper->Run();

	if (looper->Lock()) {
		error = Logging::Default()->StartWatchingConfigFile(looper);
		if (error != B_OK) {
			fprintf(stderr, "Error: Starting watching config file failed: %s\n",
				strerror(error));
		}
		looper->Unlock();
	}

	A a;
	for (int32 i = 0;; i++) {
		LOG_INFO("i: %ld\n", i);
		a.LogSomething(i);
		snooze(2000000);
	}


	return 0;
}

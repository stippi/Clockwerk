/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Logging.h"

#include <errno.h>
#include <stdio.h>

#include <new>

#include <Autolock.h>
#include <Message.h>
#include <NodeMonitor.h>

#include "ConsoleLogAppender.h"
#include "HashMap.h"
#include "HashString.h"
#include "InternalLogger.h"
#include "JavaProperties.h"
#include "PathMonitor.h"
#include "RollingFileLogAppender.h"
#include "StringTokenizer.h"


using std::nothrow;


enum {
	MSG_RELOAD_CONFIG	= 'rlcf'
};


// singleton instance
Logging* Logging::sLogging = NULL;


struct Logging::LoggerMap : HashMap<HashString, InternalLogger*> {
};


// constructor
Logging::Logging()
	: BHandler("Logging"),
	  fLock("logging"),
	  fConfiguration(NULL),
	  fLoggers(NULL),
	  fAppenders(),
	  fRootLogger(NULL),
	  fInitialized(false),
	  fReloadConfigPending(false)
{
}


// Default
Logging*
Logging::Default()
{
	if (sLogging == NULL)
		sLogging = new(nothrow) Logging;

	return sLogging;
}


// Init
status_t
Logging::Init(const char* fileName)
{
	BAutolock _(fLock);

	fConfigFileName = fileName;
	if ((size_t)fConfigFileName.Length() != strlen(fileName)) {
		fprintf(stderr, "Logging::Init(): No memory\n");
		return B_NO_MEMORY;
	}

	// create the structures, if not done yet
	if (!_IsInitialized()) {
		fConfiguration = new(nothrow) JavaProperties;
		fLoggers = new(nothrow) LoggerMap;
		fRootLogger = new(nothrow) InternalLogger("root");

		if (fConfiguration == NULL || fLoggers == NULL || fRootLogger == NULL) {
			fprintf(stderr, "Logging::Init(): No memory\n");
			return B_NO_MEMORY;
		}
	
		fInitialized = true;
	}

	_LoadConfigFile(fConfigFileName.String());

	return B_OK;
}


// InitSimple
status_t
Logging::InitSimple(int logThreshold)
{
	BAutolock _(fLock);

	// create the structures, if not done yet
	if (!_IsInitialized()) {
		fConfiguration = new(nothrow) JavaProperties;
		fLoggers = new(nothrow) LoggerMap;
		fRootLogger = new(nothrow) InternalLogger("root");

		if (fConfiguration == NULL || fLoggers == NULL || fRootLogger == NULL) {
			fprintf(stderr, "Logging::InitSimple(): No memory\n");
			return B_NO_MEMORY;
		}
	
		fInitialized = true;
	}

	// prepare a configuration for a console appender
	fConfiguration->Clear();
	fConfiguration->SetProperty("log.appender.Call", "ConsoleLogAppender");
	fConfiguration->SetProperty("log.appender.Call.threshold", "TRACE");
	fConfiguration->SetProperty("log.appender.Call.layout",
		"%d %-5p [%t] %c - %m");

	// uninit current appenders
	_UninitAppenders();

	// init a single console appender
	_InitAppender("Call");

	// init root logger
	fRootLogger->SetTo(logThreshold, fAppenders);

	// re-init other loggers
	LoggerMap::Iterator it = fLoggers->GetIterator();
	while (it.HasNext()) {
		InternalLogger* logger = it.Next().value;
		logger->SetTo(logThreshold, fAppenders);
	}

	return B_OK;
}


// StartWatchingConfigFile
status_t
Logging::StartWatchingConfigFile(BLooper* looper)
{
	BAutolock _(fLock);

	if (!looper || Looper())
		return B_BAD_VALUE;

	if (!_IsInitialized() || fConfigFileName.Length() == 0)
		return B_NO_INIT;

	// add handler
	looper->AddHandler(this);

	status_t error = BPrivate::BPathMonitor::StartWatching(
		fConfigFileName.String(),
		B_WATCH_STAT | B_WATCH_NAME | B_WATCH_FILES_ONLY,
		BMessenger(this), looper);
	if (error != B_OK) {
		looper->RemoveHandler(this);
		return error;
	}

	return B_OK;
}


// StopWatchingConfigFile
void
Logging::StopWatchingConfigFile()
{
	BAutolock _(fLock);

	if (BLooper* looper = Looper()) {
		BPrivate::BPathMonitor::StopWatching(BMessenger(this));
		looper->RemoveHandler(this);
	}
}


// LoggerFor
InternalLogger*
Logging::LoggerFor(const char* name)
{
	BAutolock _(fLock);

	if (!_IsInitialized()) {
		fprintf(stderr, "Logging::LoggerFor(\"%s\"): Not yet initialized!\n",
			name);
		return NULL;
	}

	if (name == NULL || strlen(name) == 0)
		return fRootLogger;

	InternalLogger* logger = fLoggers->Get(name);
	if (logger != NULL)
		return logger;

	int threshold = _LogLevelForCategory(name);
	logger = new(nothrow) InternalLogger(name, threshold, fAppenders);
	if (logger == NULL) {
		fprintf(stderr, "Logging::LoggerFor(): Out of memory!\n");
		return NULL;
	}

	fLoggers->Put(name, logger);

	return logger;
}


// LogLevelFor
int
Logging::LogLevelFor(const char* _level, int defaultLevel)
{
	if (_level == NULL)
		return defaultLevel;

	BString level(_level);
	if (level == "NONE")
		return LOG_LEVEL_NONE;
	if (level == "FATAL")
		return LOG_LEVEL_FATAL;
	if (level == "ERROR")
		return LOG_LEVEL_ERROR;
	if (level == "WARN")
		return LOG_LEVEL_WARN;
	if (level == "INFO")
		return LOG_LEVEL_INFO;
	if (level == "DEBUG")
		return LOG_LEVEL_DEBUG;
	if (level == "TRACE")
		return LOG_LEVEL_TRACE;

	return defaultLevel;
}


// MessageReceived
void
Logging::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_PATH_MONITOR:
		{
			BAutolock _(fLock);
			if (fReloadConfigPending)
				break;

			if (Looper()->PostMessage(MSG_RELOAD_CONFIG, this) == B_OK) {
				fReloadConfigPending = true;
			} else {
				// if sending the message failed, do it immediately
				_ReloadConfigFile();
			}

			break;
		}

		case MSG_RELOAD_CONFIG:
		{
			BAutolock _(fLock);

			fReloadConfigPending = false;
			_ReloadConfigFile();

			break;
		}

		default:
			BHandler::MessageReceived(message);
			break;
	}
}


// _LoadConfigFile
void
Logging::_LoadConfigFile(const char* fileName)
{
	// load the config file
	status_t error = fConfiguration->Load(fileName);
	if (error != B_OK) {
		fprintf(stderr, "Logging::_LoadConfigFile(): Failed to load config "
			"file: %s\n", strerror(error));
		fConfiguration->Clear();
	}

	// uninit current appenders
	_UninitAppenders();

	// get the root config and re-init root logger and appenders
	int rootLogThreshold = LOG_LEVEL_INFO;
	const char* rootLoggerConfig
		= fConfiguration->GetProperty("log.rootLogger");
	if (rootLoggerConfig != NULL) {
		StringTokenizer tokenizer(rootLoggerConfig, ",",
			StringTokenizer::kDefaultSeparators);
		BString token;

		// get root logger threshold
		if (tokenizer.NextToken(token))
			rootLogThreshold = LogLevelFor(token.String(), rootLogThreshold);

		// init appenders
		while (tokenizer.NextToken(token))
			_InitAppender(token.String());
	} else {
		fprintf(stderr, "Logging::_LoadConfigFile(): No root logger!\n");
	}

	// init root logger
	fRootLogger->SetTo(rootLogThreshold, fAppenders);

	// re-init other loggers
	LoggerMap::Iterator it = fLoggers->GetIterator();
	while (it.HasNext()) {
		InternalLogger* logger = it.Next().value;
		int threshold = _LogLevelForCategory(logger->Name());
		logger->SetTo(threshold, fAppenders);
	}
}


// _ReloadConfigFile
void
Logging::_ReloadConfigFile()
{
	_LoadConfigFile(fConfigFileName.String());

	if (fRootLogger)
		fRootLogger->Log(LOG_LEVEL_FATAL, "Logging: reloaded config file.\n");
}


// _InitAppender
status_t
Logging::_InitAppender(const char* name)
{
	BString prefix("log.appender.");
	prefix << name;

	LogAppender* appender;

	const char* type = fConfiguration->GetProperty(prefix.String());
	if (type == NULL) {
		fprintf(stderr, "Logging: No type given for appender \"%s\"\n",
			name);
		return B_BAD_VALUE;
	}

	if (strcmp(type, "ConsoleLogAppender") == 0) {
		appender = new(nothrow) ConsoleLogAppender(name);
	} else if (strcmp(type, "RollingFileLogAppender") == 0) {
		appender = new(nothrow) RollingFileLogAppender(name);
	} else {
		fprintf(stderr, "Logging: Invalid type given for appender \"%s\": "
			"\"%s\"\n", name, type);
		return B_BAD_VALUE;
	}

	if (appender == NULL) {
		fprintf(stderr, "Logging: Out of memory!\n");
		return B_NO_MEMORY;
	}

	status_t error = appender->Init(fConfiguration, prefix.String());
	if (error == B_OK) {
		if (!fAppenders.AddItem(appender))
			error = B_NO_MEMORY;
	}

	if (error != B_OK) {
		fprintf(stderr, "Logging: Error initializing appender \"%s\": %s\n",
			name, strerror(error));
		delete appender;
		return B_ERROR;
	}

	return B_OK;
}


// _UninitAppenders
void
Logging::_UninitAppenders()
{
	int32 count = fAppenders.CountItems();
	for (int32 i = 0; i < count; i++) {
		LogAppender* appender = (LogAppender*)fAppenders.ItemAt(i);
		appender->Uninit();
		appender->RemoveReference();
	}

	fAppenders.MakeEmpty();
}


// _LogLevelForCategory
int
Logging::_LogLevelForCategory(const char* name)
{
	if (name == NULL || strlen(name) == 0)
		return fRootLogger->Threshold();

	BString category("log.category.");
	int32 lastDotIndex = category.Length() - 1;
	category << name;

	while (true) {
		const char* level = fConfiguration->GetProperty(category.String());
		if (level != NULL) {
			int logLevel = LogLevelFor(level, LOG_LEVEL_INVALID);
			if (logLevel != LOG_LEVEL_INVALID)
				return logLevel;
		}

		// log level for category not specified -- try parent category
		int32 index = category.FindLast('.');
		if (index <= lastDotIndex)
			break;

		category.Truncate(index);
	}

	return fRootLogger->Threshold();
}


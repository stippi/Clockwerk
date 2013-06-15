/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOGGING_H
#define LOGGING_H

#include <Handler.h>
#include <List.h>
#include <Locker.h>
#include <String.h>


// log levels
enum {
	LOG_LEVEL_INVALID	= -2,
	LOG_LEVEL_NONE		= -1,
	LOG_LEVEL_FATAL		= 0,
	LOG_LEVEL_ERROR		= 1,
	LOG_LEVEL_WARN		= 2,
	LOG_LEVEL_INFO		= 3,
	LOG_LEVEL_DEBUG		= 4,
	LOG_LEVEL_TRACE		= 5
};


#ifndef PRINTF_LIKE
#	define PRINTF_LIKE(formatIndex, firstArg) \
		__attribute__((format(__printf__, formatIndex, firstArg)))
#endif


class InternalLogger;
class JavaProperties;


class Logging : private BHandler {
private:
								Logging();

public:
	static	Logging*			Default();

			status_t			Init(const char* fileName);
			status_t			InitSimple(int logThreshold);

			status_t			StartWatchingConfigFile(BLooper* looper);
			void				StopWatchingConfigFile();
									// looper must be locked in both cases

			InternalLogger*		LoggerFor(const char* name);
									// name must persist!

	static	int					LogLevelFor(const char* level,
									int defaultLevel);

	inline	bool				Lock()		{ return fLock.Lock(); }
	inline	void				Unlock()	{ fLock.Unlock(); }

private:
	struct LoggerMap;

	virtual	void				MessageReceived(BMessage* message);

			void				_LoadConfigFile(const char* fileName);
			void				_ReloadConfigFile();

			status_t			_InitAppender(const char* name);
			void				_UninitAppenders();

			int					_LogLevelForCategory(const char* name);

			bool				_IsInitialized() const
									{ return fInitialized; }

private:
	static	Logging*			sLogging;

			BLocker				fLock;
			BString				fConfigFileName;
			JavaProperties*		fConfiguration;
			LoggerMap*			fLoggers;
			BList				fAppenders;
			InternalLogger*		fRootLogger;
			bool				fInitialized;
			bool				fReloadConfigPending;
};


#endif	// LOGGING_H

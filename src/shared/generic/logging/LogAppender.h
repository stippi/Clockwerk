/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOG_APPENDER_H
#define LOG_APPENDER_H

#include <Locker.h>
#include <String.h>

#include "Referenceable.h"


class JavaProperties;
class LogBuffer;
class LogMessageInfo;


class LogAppender : public Referenceable {
public:
								LogAppender(const char* name);
	virtual						~LogAppender();

	virtual	status_t			Init(const JavaProperties* config,
									const char* prefix);
	virtual	void				Uninit();

			const char*			Name() const	{ return fName.String(); }

			status_t			SetLayout(const char* layout);

			void				SetThreshold(int threshold);
			int					Threshold() const	{ return fThreshold; } 

	virtual	void				AppendMessage(LogMessageInfo& info,
									const char* message);

protected:
			void				_AppendText(const char* text, size_t len,
									int logLevel);
			void				_FlushBuffer(int logLevel);

	virtual	void				PutText(const char* text, size_t len,
									int logLevel) = 0;
	virtual	void				Shutdown();

protected:
			class MessageLayout;
			struct MessageLayoutItem;

			BString				fName;
			BLocker				fLock;
			int					fThreshold;
			LogBuffer*			fLogBuffer;
			MessageLayout*		fLayout;
			bool				fNewLinePrinted;
			bool				fShutdown;
};


#endif	// LOG_APPENDER_H

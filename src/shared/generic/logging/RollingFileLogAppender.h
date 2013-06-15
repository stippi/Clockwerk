/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ROLLING_FILE_LOG_APPENDER_H
#define ROLLING_FILE_LOG_APPENDER_H

#include <File.h>
#include <String.h>

#include "LogAppender.h"


class RollingFileLogAppender : public LogAppender {
public:
								RollingFileLogAppender(const char* name);
	virtual						~RollingFileLogAppender();

	virtual	status_t			Init(const JavaProperties* config,
									const char* prefix);

protected:
	virtual	void				PutText(const char* text, size_t len,
									int logLevel);
	virtual	void				Shutdown();

private:
			status_t			_RollFiles();

private:
			BString				fFileName;
			int32				fBackupCount;
			off_t				fMaxFileSize;
			BFile				fFile;
			off_t				fFileSize;
			int32				fLastBackupIndex;
			bool				fError;
};


#endif	// ROLLING_FILE_LOG_APPENDER_H

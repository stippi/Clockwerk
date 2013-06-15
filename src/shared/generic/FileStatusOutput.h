/*
 * Copyright 2006-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <File.h>
#include <Locker.h>

#include "StatusOutput.h"


class FileStatusOutput : public StatusOutput {
public:
								FileStatusOutput(const char* pathToFile);
	virtual						~FileStatusOutput();

	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);
	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* message);

	// FileStatusOutput
	enum log_level {
		LOG_ERRORS		= 0x01,
		LOG_WARNINGS	= 0x02,
		LOG_INFOS		= 0x04,

		LOG_ALL			= LOG_ERRORS | LOG_WARNINGS | LOG_INFOS
	};

			void				SetLogLevel(enum log_level level);
			status_t			InitCheck() const
									{ return fLogFile.InitCheck(); }

private:
			void				_Output(const char* header,
									const char* message);

			BFile				fLogFile;
			int32				fLogLevel;
			BLocker				fLock;

			bool				fNeedsHeader;
};



/*
 * Copyright 2002-2007, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef STATUS_OUTPUT_H
#define STATUS_OUTPUT_H

#include <stdarg.h>

#include <Messenger.h>


// types of status messages
enum {
	STATUS_MESSAGE_INFO,
	STATUS_MESSAGE_WARNING,
	STATUS_MESSAGE_ERROR,
};


// StatusOutput
class StatusOutput {
public:
	virtual						~StatusOutput();

			void				PrintInfoMessage(const char* format, ...);
			void				PrintWarningMessage(const char* format, ...);
			void				PrintErrorMessage(const char* format, ...);

			void				PrintStatusMessage(uint32 type,
									const char* format, ...);

	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);
	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* message) = 0;
};


// NoneStatusOutput
class NoneStatusOutput : public StatusOutput {
public:
	virtual						~NoneStatusOutput();

	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);
	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* message);
};


// ConsoleStatusOutput
class ConsoleStatusOutput : public StatusOutput {
public:
	virtual						~ConsoleStatusOutput();

	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);
	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* message);
};


// MessageStatusOutput
class MessageStatusOutput : public StatusOutput {
public:
								MessageStatusOutput(BMessenger target,
									uint32 messageCommand);
	virtual						~MessageStatusOutput();

	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);
	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* messageText);

private:
			BMessenger			fTarget;
			uint32				fMessageCommand;
};


#endif	// STATUS_OUTPUT_H

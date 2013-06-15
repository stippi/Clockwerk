/*
 * Copyright 2002-2007, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StatusOutput.h"

#include <stdio.h>

#include <Message.h>


// #pragma mark - StatusOuptut

// destructor
StatusOutput::~StatusOutput()
{
}

// PrintInfoMessage
void
StatusOutput::PrintInfoMessage(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	InternalPrintStatusMessage(STATUS_MESSAGE_INFO, format, list);
	va_end(list);
}

// PrintWarningMessage
void
StatusOutput::PrintWarningMessage(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	InternalPrintStatusMessage(STATUS_MESSAGE_WARNING, format, list);
	va_end(list);
}

// PrintErrorMessage
void
StatusOutput::PrintErrorMessage(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	InternalPrintStatusMessage(STATUS_MESSAGE_ERROR, format, list);
	va_end(list);
}

// PrintStatusMessage
void
StatusOutput::PrintStatusMessage(uint32 type, const char* format, ...)
{
	va_list list;
	va_start(list, format);
	InternalPrintStatusMessage(type, format, list);
	va_end(list);
}

// InternalPrintStatusMessage
void
StatusOutput::InternalPrintStatusMessage(uint32 type, const char* format,
	va_list list)
{
	char buffer[1024];
	vsnprintf(buffer, sizeof(buffer), format, list);
	InternalPrintStatusMessage(type, buffer);
}


// #pragma mark - NoneStatusOutput


// destructor
NoneStatusOutput::~NoneStatusOutput()
{
}


// InternalPrintStatusMessage
void
NoneStatusOutput::InternalPrintStatusMessage(uint32 type, const char* format,
	va_list list)
{
}


// InternalPrintStatusMessage
void
NoneStatusOutput::InternalPrintStatusMessage(uint32 type, const char* message)
{
}


// #pragma mark - ConsoleStatusOutput

// destructor
ConsoleStatusOutput::~ConsoleStatusOutput()
{
}

// InternalPrintStatusMessage
void
ConsoleStatusOutput::InternalPrintStatusMessage(uint32 type, const char* format,
	va_list list)
{
	StatusOutput::InternalPrintStatusMessage(type, format, list);
}

// InternalPrintStatusMessage
void
ConsoleStatusOutput::InternalPrintStatusMessage(uint32 type,
	const char* message)
{
	if (type == STATUS_MESSAGE_ERROR) {
		fprintf(stderr, "%s", message);
		fflush(stderr);
	} else {
		printf("%s", message);
		fflush(stdout);
	}
}


// #pragma mark - MessageStatusOutput

// constructor
MessageStatusOutput::MessageStatusOutput(BMessenger target,
		uint32 messageCommand)
	: fTarget(target),
	  fMessageCommand(messageCommand)
{
}

// destructor
MessageStatusOutput::~MessageStatusOutput()
{
}

// InternalPrintStatusMessage
void
MessageStatusOutput::InternalPrintStatusMessage(uint32 type, const char* format,
	va_list list)
{
	StatusOutput::InternalPrintStatusMessage(type, format, list);
}

// InternalPrintStatusMessage
void
MessageStatusOutput::InternalPrintStatusMessage(uint32 type,
	const char* messageText)
{
	BMessage message(fMessageCommand);
	message.AddInt32("type", type);
	message.AddString("message", messageText);
	fTarget.SendMessage(&message);
}

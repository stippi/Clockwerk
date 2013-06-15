/*
 * Copyright 2006-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "FileStatusOutput.h"

#include <stdio.h>
#include <unistd.h>

#include <Autolock.h>
#include <NodeInfo.h>
#include <String.h>


FileStatusOutput::FileStatusOutput(const char* pathToFile)
	: StatusOutput()
	, fLogFile(pathToFile, B_WRITE_ONLY | B_CREATE_FILE)
	, fLogLevel(LOG_ALL)
	, fNeedsHeader(true)
{
	BNodeInfo info(&fLogFile);
	info.SetType("text/plain");

	fLogFile.Seek(0, SEEK_END);
	_Output("START   ", "\n");
}


FileStatusOutput::~FileStatusOutput()
{
	_Output("END     ", "\n\n");
}


void
FileStatusOutput::InternalPrintStatusMessage(uint32 type, const char* format,
	va_list list)
{
	StatusOutput::InternalPrintStatusMessage(type, format, list);
}


void
FileStatusOutput::InternalPrintStatusMessage(uint32 type, const char* message)
{
	BAutolock _(fLock);

	const char* name = NULL;
	switch (type) {
		case STATUS_MESSAGE_INFO:
			if (fLogLevel & LOG_INFOS)
				name = "INFO    ";
			else
				return;
			break;
		case STATUS_MESSAGE_WARNING:
			if (fLogLevel & LOG_WARNINGS)
				name = "WARNING ";
			else
				return;
			break;
		case STATUS_MESSAGE_ERROR:
			if (fLogLevel & LOG_ERRORS)
				name = "ERROR   ";
			else
				return;
			break;
	}
	_Output(name, message);
}


void
FileStatusOutput::SetLogLevel(enum log_level level)
{
	BAutolock _(fLock);

	fLogLevel = level;
}


void
FileStatusOutput::_Output(const char* _header, const char* message)
{
	if (fLogFile.InitCheck() < B_OK)
		return;

	off_t size;
	if (fLogFile.GetSize(&size) == B_OK) {
		if (size > 10 * 1024 * 1024)
			fLogFile.Seek(0, SEEK_SET);
	}

	bool prependLineBreak = false;
	if (message[0] == '\n') {
		message++;
		fNeedsHeader = true;
		prependLineBreak = true;
	}

	if (fNeedsHeader) {
		time_t nowSeconds = time(NULL);
		tm now = *localtime(&nowSeconds);
		char time[256];
		strftime(time, 256, "%d.%m.%Y, %H:%M:%S", &now);
	
		char header[512];
		if (prependLineBreak)
			sprintf(header, "\n%s[%s]  ", _header, time);
		else
			sprintf(header, "%s[%s]  ", _header, time);
	
		fLogFile.Write(header, strlen(header));
	}

	BString helper(message);
	fNeedsHeader = helper.FindFirst('\n') >= 0;

	fLogFile.Write(message, strlen(message));

	sync();
}



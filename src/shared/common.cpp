/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "common.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <Path.h>
#include <String.h>

#include "CommonPropertyIDs.h"
#include "ServerObject.h"

// fatal
void
fatal(status_t error)
{
	print_error("program terminated with fatal error: %s\n", strerror(error));
	debugger("fatal error encountered");
}

// copy_data
off_t
copy_data(BDataIO& source, BDataIO& destination, off_t maxSize)
{
	ssize_t bufferSize = 64 * 1024;
	char buffer[bufferSize];
	off_t copied = 0;
	if (maxSize < 0)
		maxSize = LONG_LONG_MAX;

	while (maxSize > 0) {
		size_t toRead = (size_t)min_c(maxSize, bufferSize);
		ssize_t read = source.Read(buffer, toRead);
		if (read <= 0) {
			if (read < 0)
				return read;
			break;
		}
		ssize_t written = destination.Write(buffer, read);
		if (written != read) {
			return written < 0 ? written : B_ERROR;
		}
		copied += read;
		maxSize -= read;
	}
	return copied;
}

class GlobalOutputMaintainer : public BLocker {
public:
	GlobalOutputMaintainer()
		: BLocker("status output maintainer lock")
		, fConsolOutput()
		, fOutput(&fConsolOutput)
	{
	}
	~GlobalOutputMaintainer()
	{
	}
	StatusOutput* Output() const
	{
		return fOutput;
	}
	void SetOutput(StatusOutput* output)
	{
		fOutput = output;
	}
private:
	ConsoleStatusOutput fConsolOutput;
	StatusOutput* fOutput;
};

// global status output maintainer
static GlobalOutputMaintainer gOutputMaintainer;

void
set_global_output(StatusOutput* statusOutput)
{
	BAutolock _(gOutputMaintainer);
	gOutputMaintainer.SetOutput(statusOutput);
}


// print_info
void
print_info(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	BAutolock _(gOutputMaintainer);
	gOutputMaintainer.Output()->InternalPrintStatusMessage(STATUS_MESSAGE_INFO, format, list);
	va_end(list);
}

// print_warning
void
print_warning(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	BAutolock _(gOutputMaintainer);
	gOutputMaintainer.Output()->InternalPrintStatusMessage(STATUS_MESSAGE_WARNING, format, list);
	va_end(list);
}

// print_error
void
print_error(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	BAutolock _(gOutputMaintainer);
	gOutputMaintainer.Output()->InternalPrintStatusMessage(STATUS_MESSAGE_ERROR, format, list);
	va_end(list);
}

/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RollingFileLogAppender.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "JavaProperties.h"


static const off_t kMinFileSize = 1024;
static const off_t kMaxFileSize = 1024LL * 1024 * 1024;
static const int32 kMaxBackupCount = 99;


// constructor
RollingFileLogAppender::RollingFileLogAppender(const char* name)
	: LogAppender(name),
	  fFileName(),
	  fBackupCount(0),
	  fMaxFileSize(0),
	  fFile(),
	  fFileSize(0),
	  fLastBackupIndex(0),
	  fError(true)
{
}


// destructor
RollingFileLogAppender::~RollingFileLogAppender()
{
}


// Init
status_t
RollingFileLogAppender::Init(const JavaProperties* config, const char* prefix)
{
	status_t error = LogAppender::Init(config, prefix);
	if (error != B_OK)
		return error;

	// file name

	const char* fileName
		= config->GetProperty((BString(prefix) << ".file").String());
	if (fileName == NULL || strlen(fileName) == 0) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"No file name given!\n", Name());
		return B_BAD_VALUE;
	}

	// max file size

	const char* maxFileSize
		= config->GetProperty((BString(prefix) << ".maxFileSize").String());
	if (maxFileSize == NULL || strlen(maxFileSize) == 0) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"No maximal file size given!\n", Name());
		return B_BAD_VALUE;
	}

	// skip whitespace
	while (isspace(*maxFileSize))
		maxFileSize++;

	// get the number
	int32 endIndex = 0;
	while (isdigit(maxFileSize[endIndex]))
		endIndex++;

	if (endIndex == 0) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"Invalid file size specification given!\n", Name());
		return B_BAD_VALUE;
	}

	off_t size = atoll(BString(maxFileSize, endIndex).String());

	// skip whitespace
	maxFileSize += endIndex;
	while (isspace(*maxFileSize))
		maxFileSize++;

	// get the size modifier
	if (*maxFileSize != '\0') {
		switch (*maxFileSize) {
			case 'K':
				size *= 1024;
				break;
			case 'M':
				size *= 1024 * 1024;
				break;
			case 'G':
				size *= 1024LL * 1024 * 1024;
				break;
			default:
				maxFileSize--;
				break;
		}

		maxFileSize++;
	}

	if (*maxFileSize == 'B')
		maxFileSize++;

	// skip whitespace
	while (isspace(*maxFileSize))
		maxFileSize++;

	if (*maxFileSize != '\0') {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"Invalid file size specification given!\n", Name());
		return B_BAD_VALUE;
	}

	if (size < kMinFileSize || size > kMaxFileSize) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"Invalid file size given!\n", Name());
		return B_BAD_VALUE;
	}

	fMaxFileSize = size;

	// maxBackupIndex
	const char* maxBackupIndex
		= config->GetProperty((BString(prefix) << ".maxBackupIndex").String());
	if (maxBackupIndex == NULL || strlen(maxBackupIndex) == 0) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"No maximal backup index given!\n", Name());
		return B_BAD_VALUE;
	}

	fBackupCount = atol(maxBackupIndex);
	if (fBackupCount < 1 || fBackupCount > kMaxBackupCount) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender \"%s\": "
			"Invalid maximal backup index given: %ld\n", Name(), fBackupCount);
		return B_BAD_VALUE;
	}

	// create log file directory, if necessary
	char* lastSlash = strrchr(fileName, '/');
	if (lastSlash != NULL && lastSlash != fileName) {
		BString dirPath(fileName, lastSlash - fileName);
		if (error == B_OK) {
			error = create_directory(dirPath.String(),
				S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
		}
		if (error != B_OK) {
			fprintf(stderr, "Error: Failed to init RollingFileLogAppender "
				"\"%s\": Failed to init logging directory: %s\n", Name(),
				strerror(error));
			return error;
		}
	}

	// open log file
	BPath path;
	error = path.SetTo(fileName, NULL, true);
	if (error == B_OK) {
		fFileName = path.Path();
		if (fFileName.Length() == 0)
			return B_NO_MEMORY;
	}
	if (error == B_OK) {
		error = fFile.SetTo(path.Path(),
			B_WRITE_ONLY | B_OPEN_AT_END | B_CREATE_FILE);
	}
	if (error == B_OK)
		error = fFile.GetSize(&fFileSize);
	if (error != B_OK) {
		fprintf(stderr, "Error: Failed to init RollingFileLogAppender "
			"\"%s\": Failed to open log file: %s\n", Name(),
			strerror(error));
		return error;
	}

	// determine the greatest log file index
	fLastBackupIndex = 0;
	for (int32 i = fBackupCount; i > 0; i--) {
		char suffix[16];
		sprintf(suffix, ".%02ld", i);
		if (BEntry((BString(path.Path()) << suffix).String()).Exists()) {
			fLastBackupIndex = i;
			break;
		}
	}

	fError = false;

	return B_OK;
}


// PutText
void
RollingFileLogAppender::PutText(const char* text, size_t len, int logLevel)
{
	if (fError)
		return;

	// roll the files, if necessary
	if (fFileSize + len > fMaxFileSize) {
		if (_RollFiles() != B_OK) {
			fError = true;
			return;
		}
	}

	// write to file
	ssize_t bytesWritten = fFile.Write(text, len);
	if (bytesWritten < 0) {
		fprintf(stderr, "Error: Appender \"%s\" failed to write to log file: "
			"%s\n", Name(), strerror(bytesWritten));
		fError = true;
	} else
		fFileSize += bytesWritten;
}


// _RollFiles
status_t
RollingFileLogAppender::_RollFiles()
{
	for (int32 i = fLastBackupIndex; i >= 0; i--) {
		// construct current and new file name
		char currentSuffix[16];
		char newSuffix[16];
		sprintf(currentSuffix, ".%02ld", i);
		sprintf(newSuffix, ".%02ld", i + 1);

		BString currentName(fFileName);
		BString newName(fFileName);
		if (i > 0)
			currentName << currentSuffix;
		newName << newSuffix;

		if (currentName.Length() == 0 || (i > 0 && currentName == fFileName)
			|| newName.Length() == 0 || newName == fFileName) {
			return B_NO_MEMORY;
		}

		// init entry
		BEntry entry;
		status_t error = entry.SetTo(currentName.String());
		if (error != B_OK || !entry.Exists())
			continue;

		// delete the last, otherwise rename
		if (i == fBackupCount)
			error = entry.Remove();
		else
			error = entry.Rename(newName.String(), true);

		if (error != B_OK) {
			fprintf(stderr, "Error: appender \"%s\": Failed to rename/remove "
				"log file \"%s\": %s\n", Name(), currentName.String(),
				strerror(error));
		}
	}

	// close the current log file and reopen it
	fFile.Unset();
	status_t error = fFile.SetTo(fFileName.String(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (error != B_OK) {
		fprintf(stderr, "Error: Appender \"%s\" failed to write to log file: "
			"%s\n", Name(), strerror(error));
		return error;
	}

	fFileSize = 0;
	if (fLastBackupIndex < fBackupCount)
		fLastBackupIndex++;

	return B_OK;
}


// Shutdown
void
RollingFileLogAppender::Shutdown()
{
	fFile.Unset();
}

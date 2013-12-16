/*
 * Copyright 2006-2008, Ingo Weinhold <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "LogAppender.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include <new>

#include <Autolock.h>
#include <List.h>

#include "AutoDeleter.h"
#include "JavaProperties.h"
#include "LogBuffer.h"
#include "Logging.h"
#include "LogMessageInfo.h"


using std::nothrow;

static const size_t kLogBufferCapacity	= 10240;


enum layout_item_type {
	LAYOUT_TEXT,
	LAYOUT_MESSAGE,
	LAYOUT_TIME,
	LAYOUT_CLASS,
	LAYOUT_THREAD,
	LAYOUT_LOG_LEVEL,
};


// MessageLayoutItem
struct LogAppender::MessageLayoutItem {
	MessageLayoutItem()
		: type(LAYOUT_TEXT),
		  text(NULL)
	{
	}

	MessageLayoutItem(layout_item_type type, char* text)
		: type(type),
		  text(text)
	{
	}

	~MessageLayoutItem()
	{
		free(text);
	}

	layout_item_type	type;
	char*				text;
};


// MessageLayout
class LogAppender::MessageLayout {
public:
	MessageLayout()
	{
	}

	status_t Init(const char* appenderName, const char* pattern)
	{
		int patternLen = strlen(pattern);
		int index = 0;
		while (const char* percent = strchr(pattern + index, '%')) {
			int percentIndex = percent - pattern;
			if (percentIndex > index) {
				if (!_AppendTextItem(pattern + index, percentIndex - index))
					return B_NO_MEMORY;
			}

			MessageLayoutItem* item = NULL;
			char placeholder = 0;
			ssize_t placeholderLen = _ParsePlaceholder(percent, placeholder,
				item);
			if (placeholderLen < 0) {
				if (placeholderLen == B_BAD_VALUE) {
					fprintf(stderr, "Error: Bad layout specification for "
						"appender \"%s\"\n", appenderName);
				}
				return placeholderLen;
			}

			switch (placeholder) {
				case 'c':
					item->type = LAYOUT_CLASS;
					break;
				case 'd':
					item->type = LAYOUT_TIME;
					break;
				case 'm':
					item->type = LAYOUT_MESSAGE;
					break;
				case 'p':
					item->type = LAYOUT_LOG_LEVEL;
					break;
				case 't':
					item->type = LAYOUT_THREAD;
					break;
				default:
					fprintf(stderr, "Unknown placeholder character \"%c\" in "
						"layout specification for appender \"%s\"\n",
						placeholder, appenderName);
					delete item;
					item = NULL;
					break;
			}

			if (item != NULL) {
				if (!fItems.AddItem(item)) {
					delete item;
					return B_NO_MEMORY;
				}
			}

			index = percentIndex + placeholderLen;
		}

		if (index < patternLen) {
			if (!_AppendTextItem(pattern + index, patternLen - index))
				return B_NO_MEMORY;
		}

		return B_OK;
	}

	int32 CountItems() const
	{
		return fItems.CountItems();
	}

	const char* FormatItem(int32 index, LogMessageInfo& info,
		const char* message, char* buffer, size_t bufferSize)
	{
		MessageLayoutItem* item = (MessageLayoutItem*)fItems.ItemAt(index);
		if (item == NULL)
			return NULL;

		const char* text = NULL;
		const char* format = item->text;

		switch (item->type) {
			case LAYOUT_TEXT:
				text = item->text;
				format = NULL;
				break;
			case LAYOUT_MESSAGE:
				text = message;
				format = NULL;
				break;
			case LAYOUT_TIME:
			{
				const struct tm* time = info.Time();
				int32 timeMicros = info.TimeMicros();
				snprintf(buffer, bufferSize,
					"%d-%02d-%02d %02d:%02d:%02d.%03ld",
					time->tm_year + 1900, time->tm_mon + 1, time->tm_mday,
					time->tm_hour, time->tm_min, time->tm_sec,
					timeMicros / 1000);
				text = buffer;
				break;
			}
			case LAYOUT_CLASS:
				text = info.Name();
				break;
			case LAYOUT_THREAD:
				text = info.ThreadName();
				break;
			case LAYOUT_LOG_LEVEL:
				switch (info.LogLevel()) {
					case LOG_LEVEL_FATAL:
						text = "FATAL";
						break;
					case LOG_LEVEL_ERROR:
						text = "ERROR";
						break;
					case LOG_LEVEL_WARN:
						text = "WARN";
						break;
					case LOG_LEVEL_INFO:
						text = "INFO";
						break;
					case LOG_LEVEL_DEBUG:
						text = "DEBUG";
						break;
					case LOG_LEVEL_TRACE:
						text = "TRACE";
						break;
					default:
						snprintf(buffer, bufferSize, "%d", info.LogLevel());
						text = buffer;
						break;
				}
				break;
		}

		// format the string, if a format is given
		if (text != NULL && format != NULL && text != buffer) {
			snprintf(buffer, bufferSize, format, text);
			text = buffer;
		}

		return text;
	}

private:
	char* _CloneString(const char* text, size_t len)
	{
		char* clonedText = (char*)malloc(len + 1);
		if (clonedText != NULL) {
			memcpy(clonedText, text, len);
			clonedText[len] = '\0';
		}
		return clonedText;
	}

	bool _AppendTextItem(const char* text, size_t len)
	{
		char* itemText = _CloneString(text, len);
		if (!itemText)
			return false;

		MessageLayoutItem* item = new(nothrow) MessageLayoutItem(LAYOUT_TEXT,
			itemText);
		if (item == NULL) {
			free(itemText);
			return false;
		}

		if (!fItems.AddItem(item)) {
			delete item;
			return false;
		}

		return true;
	}


	ssize_t _ParsePlaceholder(const char* text, char& placeholder,
		MessageLayoutItem*& _item)
	{
		// allocate an item
		MessageLayoutItem* item = new(nothrow) MessageLayoutItem;
		if (item == NULL)
			return false;
		ObjectDeleter<MessageLayoutItem> itemDeleter(item);

		const char* textStart = text;

		bool keepFormat = false;

		// skip '%'
		text++;

		// skip '-'
		if (*text == '-') {
			// left aligned
			text++;
		}

		// skip field width
		while (isdigit(*text)) {
			text++;
			keepFormat = true;
		}

		// skip field precision
		if (*text == '.') {
			text++;
			while (isdigit(*text)) {
				text++;
				keepFormat = true;
			}
		}

		// now we should have a character indicating what is to be printed
		if (!isalpha(*text))
			return B_BAD_VALUE;

		placeholder = *text;
		text++;
		ssize_t len = text - textStart;

		if (keepFormat) {
			char* format = _CloneString(textStart, len);
			if (format == NULL)
				return B_NO_MEMORY;

			// replace format char by 's'
			format[len - 1] = 's';

			item->text = format;
		}

		itemDeleter.Detach();
		_item = item;

		return len;
	}

private:
	BList	fItems;
};



// constructor
LogAppender::LogAppender(const char* name)
	: Referenceable(true),
	  fName(name),
	  fLock(name),
	  fThreshold(LOG_LEVEL_INFO),
	  fLogBuffer(NULL),
	  fLayout(NULL),
	  fNewLinePrinted(false),
	  fShutdown(false)
{
}


// destructor
LogAppender::~LogAppender()
{
	delete fLayout;
	delete fLogBuffer;
}


// Init
status_t
LogAppender::Init(const JavaProperties* config, const char* prefix)
{
	if (fLock.Sem() < 0)
		return fLock.Sem();

	fLogBuffer = new(nothrow) LogBuffer;
	if (fLogBuffer == NULL)
		return B_NO_MEMORY;

	status_t error = fLogBuffer->Init(kLogBufferCapacity);
	if (error != B_OK)
		return error;

	// configuration

	// threshold
	const char* threshold
		= config->GetProperty((BString(prefix) << ".threshold").String());
	fThreshold = Logging::LogLevelFor(threshold, LOG_LEVEL_TRACE);

	// layout
	const char* layout
		= config->GetProperty((BString(prefix) << ".layout").String());
	if (layout != NULL)
		SetLayout(layout);

	return B_OK;
}


// Uninit
void
LogAppender::Uninit()
{
	BAutolock _(fLock);

	if (fShutdown)
		return;

	Shutdown();
	fShutdown = true;
}


// SetLayout
status_t
LogAppender::SetLayout(const char* layout)
{
	BAutolock _(fLock);

	MessageLayout* newLayout = new(nothrow) MessageLayout;
	if (!newLayout)
		return B_NO_MEMORY;

	status_t error = newLayout->Init(fName.String(), layout);
	if (error != B_OK)
		return error;

	delete fLayout;
	fLayout = newLayout;

	return B_OK;
}


// SetThreshold
void
LogAppender::SetThreshold(int threshold)
{
	BAutolock _(fLock);

	fThreshold = threshold;
}


// AppendMessage
void
LogAppender::AppendMessage(LogMessageInfo& info, const char* message)
{
	BAutolock _(fLock);

	if (fShutdown || info.LogLevel() > fThreshold)
		return;

	// fast path for no layout
	if (fLayout == NULL) {
		PutText(message, strlen(message), info.LogLevel());
		return;
	}

	// format the message
	fNewLinePrinted = false;

	int32 count = fLayout->CountItems();
	char buffer[128];

	for (int32 i = 0; i < count; i++) {
		const char* toPrint = fLayout->FormatItem(i, info, message,
			buffer, sizeof(buffer));
		_AppendText(toPrint, strlen(toPrint), info.LogLevel());
	}

	if (!fNewLinePrinted)
		_AppendText("\n", 1, info.LogLevel());

	_FlushBuffer(info.LogLevel());
}


// _AppendText
void
LogAppender::_AppendText(const char* text, size_t len, int logLevel)
{
	if (text == NULL || len == 0)
		return;

	if (!fLogBuffer->Append(text, len)) {
		// didn't fit into the buffer anymore
		bool bufferTooSmall = true;
		if (fLogBuffer->Size() > 0) {
			_FlushBuffer(logLevel);
			bufferTooSmall = !fLogBuffer->Append(text, len);
		}

		if (bufferTooSmall) {
			// buffer is empty, but the text doesn't fit -- print it
			// directly
			PutText(text, len, logLevel);
		}
	}

	fNewLinePrinted = (text[len - 1] == '\n');
}


// _FlushBuffer
void
LogAppender::_FlushBuffer(int logLevel)
{
	PutText(fLogBuffer->Buffer(), fLogBuffer->Size(), logLevel);
	fLogBuffer->Clear();
}


// Shutdown
void
LogAppender::Shutdown()
{
	// implemented by derived classes, if anything has to be done
}

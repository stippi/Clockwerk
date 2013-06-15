/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ArgumentVector.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <String.h>


static const char* const* kNoArgs = { NULL };


// ArgvParser
class ArgumentVector::ArgvParser {
public:
	ArgvParser(ArgumentVector& vector)
		: fVector(vector)
	{
	}

	void Parse(const char* args)
	{
		while (*args != '\0') {
			if (isspace(*args)) {
				// commit the current arg
				args++;
				_CommitCurrentArg();
			} else {
				switch (*args) {
					case '\'':
					case '"':
						// string
						_ParseString(args);
						break;
					case '\\':
						// quoted char
						args++;
						_AppendChar(*args);
						args++;
						break;
					default:
						// ordinary non-whitespace char
						_AppendChar(*args);
						args++;
						break;
				}
			}
		}

		// commit last argument
		_CommitCurrentArg();
	}

private:
	void _ParseString(const char*& args)
	{
		char quoteChar = *args;
		args++;

		while (*args != quoteChar) {
			// quoted char in "..." string
			if (quoteChar == '"' && *args == '\\') {
				// skip the '\'
				args++;
			}

			_AppendChar(*args);
			args++;
		}

		// skip closing quote char
		args++;
	}

	void _AppendChar(const char& c)
	{
		if (c == '\0')
			throw status_t(0);	// unexpected end of string

		// grrr, no error reporting from BString
		int32 len = fCurrentArg.Length();
		fCurrentArg += c;
		if (fCurrentArg.Length() != len + 1)
			throw status_t(B_NO_MEMORY);
	}

	void _CommitCurrentArg()
	{
		if (fCurrentArg.Length() == 0)
			return;

		status_t error = fVector._AppendArg(fCurrentArg.String());
		if (error != B_OK)
			throw error;

		fCurrentArg.Truncate(0);
	}

private:
	ArgumentVector&	fVector;
	BString			fCurrentArg;
};


// constructor
ArgumentVector::ArgumentVector()
{
}

// destructor
ArgumentVector::~ArgumentVector()
{
}

// Arguments
const char* const*
ArgumentVector::Arguments() const
{
	return (fArguments.IsEmpty()
		? kNoArgs : (const char* const*)fArguments.Items());
}

// ArgumentCount
int32
ArgumentVector::ArgumentCount() const
{
	return fArguments.CountItems();
}

// Parse
status_t
ArgumentVector::Parse(const char* args, BString* errorMessage)
{
	if (!args)
		return B_BAD_VALUE;

	Clear();

	status_t result = B_OK;
	ArgvParser parser(*this);
	try {
		parser.Parse(args);
	} catch (status_t error) {
		if (error == 0) {
			// unexpected end of string
			if (errorMessage)
				*errorMessage = "Unexpected end of string";
			result = B_BAD_VALUE;
		} else {
			if (errorMessage)
				*errorMessage = strerror(error);
			result = error;
		}
	}

	if (result != B_OK)
		Clear();

	return result;
}

// Clear
void
ArgumentVector::Clear()
{
	int32 count = fArguments.CountItems();
	for (int32 i = 0; i < count; i++)
		free(fArguments.ItemAt(i));
	fArguments.MakeEmpty();
}

// _AppendArg
status_t
ArgumentVector::_AppendArg(const char* arg)
{
	char* argCopy = strdup(arg);
	if (!argCopy)
		return B_NO_MEMORY;

	if (!fArguments.AddItem(argCopy)) {
		free(argCopy);
		return B_NO_MEMORY;
	}

	return B_OK;
}

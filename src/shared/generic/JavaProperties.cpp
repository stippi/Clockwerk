/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "JavaProperties.h"

#include <ctype.h>
#include <stdio.h>

#include <new>

#include <File.h>
#include <String.h>


using std::nothrow;

static const size_t kBufferSize = 1024;


// Parser
class JavaProperties::Parser {
public:
	enum parse_exception {
		PARSE_READ_ERROR,
		PARSE_EOF,
		PARSE_PROGRAM_ERROR,
	};

	Parser(BDataIO& input)
		: fInput(input),
		  fBufferCapacity(0),
		  fBuffer(NULL),
		  fBufferSize(0),
		  fIndex(0),
		  fEOF(false)
	{
	}

	~Parser()
	{
		delete[] fBuffer;
	}

	status_t Parse(JavaProperties& properties)
	{
		fBuffer = new(nothrow) char[kBufferSize];
		if (fBuffer == NULL)
			return B_NO_MEMORY;

		fBufferCapacity = kBufferSize;

		try {
			while (true) {
				_SkipCommentsAndWhiteSpace();
				if (!_HasInput())
					return B_OK;

				BString propertyName;
				BString propertyValue;

				_ParsePropertyName(propertyName);
				_ParsePropertyValue(propertyValue);

				if (!properties.SetProperty(propertyName.String(),
						propertyValue.String())) {
					return B_NO_MEMORY;
				}
			}
		} catch (parse_exception exception) {
			switch (exception) {
				case PARSE_READ_ERROR:
					return B_IO_ERROR;
				case PARSE_EOF:
					fprintf(stderr, "Error: Unexpected EOF reading property "
						"file.\n");
					break;
				case PARSE_PROGRAM_ERROR:
					fprintf(stderr, "Error: Program error reading property "
						"file.\n");
					break;
			}
		}

		return B_OK;
	}

private:
	void _ParsePropertyName(BString& propertyName)
	{
		// read until we hit a '='
		while (true) {
			char c = _NextChar();
			if (c == '=')
				break;
			else if (c == '\\')
				c = _NextChar();

			propertyName << c;
		}
	}

	void _ParsePropertyValue(BString& propertyValue)
	{
		// read until we hit a '\n'
		while (true) {
			char c = _NextChar();
			if (c == '\n')
				break;
			else if (c == '\\')
				c = _NextChar();

			propertyValue << c;
		}
	}


	void _SkipCommentsAndWhiteSpace()
	{
		while (_HasInput()) {
			if (!isspace(_NextChar())) {
				if (_CurrentChar() != '#') {
					_PutChar();
					break;
				}

				// a comment -- skip the rest of the line
				while (_HasInput() && _NextChar() != '\n');
			}
		}
	}

	bool _HasInput()
	{
		if (fIndex >= fBufferSize)
			_ReadNextBuffer();

		return (fIndex < fBufferSize);
	}

	void _PutChar()
	{
		if (fIndex <= 0)
			throw PARSE_PROGRAM_ERROR;
		fIndex--;
	}

	char _CurrentChar()
	{
		int32 index = fIndex - 1;
		if (index < 0 || index >= fBufferSize)
			throw PARSE_PROGRAM_ERROR;

		return fBuffer[index];
	}

	char _NextChar(parse_exception exception = PARSE_EOF)
	{
		if (!_HasInput())
			throw exception;

		char c = fBuffer[fIndex++];

		if (c == '\r') {
			if (_HasInput() && fBuffer[fIndex] == '\n') {
				// "\r\n" -> "\n"
				c = '\n';
				fIndex++;
			} else {
				// single '\r' -- just return it
			}
		}

		return c;
	}

	void _ReadNextBuffer()
	{
		if (fEOF)
			return;

		// Keep the last character of the previous buffer. This makes it easier
		// to peek one char ahead.
		int offset = 0;
		char lastChar = '\0';
		if (fBufferSize > 0) {
			lastChar = fBuffer[fBufferSize - 1];
			offset = 1;
		}

		ssize_t bytesRead = fInput.Read(fBuffer + offset,
			fBufferCapacity - offset);
		if (bytesRead < 0)
			throw PARSE_READ_ERROR;

		if (bytesRead == 0) {
			fEOF = true;
			return;
		}

		fBufferSize = bytesRead + offset;
		fIndex = offset;
		if (offset > 0)
			fBuffer[fIndex] = lastChar;
	}

private:
	BDataIO&	fInput;
	size_t		fBufferCapacity;
	char*		fBuffer;
	int32		fBufferSize;
	int32		fIndex;
	bool		fEOF;
};


// constructor
JavaProperties::JavaProperties()
	: fParent(NULL)
{
}


// constructor
JavaProperties::JavaProperties(JavaProperties* parent)
	: fParent(parent)
{
}


// destructor
JavaProperties::~JavaProperties()
{
	delete fParent;
}


// Load
status_t
JavaProperties::Load(const char* fileName)
{
	BFile file;
	status_t error = file.SetTo(fileName, B_READ_ONLY);
	if (error != B_OK)
		return error;

	return Load(file);
}


// Load
status_t
JavaProperties::Load(BDataIO& input)
{
	Clear();

	return Parser(input).Parse(*this);
}


// Save
status_t
JavaProperties::Save(const char* fileName)
{
	BFile file;
	status_t error = file.SetTo(fileName,
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (error != B_OK)
		return error;

	return Save(file);
}


// Save
status_t
JavaProperties::Save(BDataIO& output)
{
	// Not yet supported.
	return B_BAD_VALUE;
}


// GetProperty
const char*
JavaProperties::GetProperty(const char* name, const char* defaultValue) const
{
	if (name == NULL)
		return defaultValue;

	if (ContainsKey(name)) {
		// Note: This is safe due to String using common ref-counted memory
		// when copied. Thus the const char pointer the temporary object returns
		// remains valid as long as the value remains in the hash map.
		return Get(name).GetString();
	}

	return (fParent != NULL ? fParent->GetProperty(name, defaultValue) : NULL);
}


// SetProperty
bool
JavaProperties::SetProperty(const char* _name, const char* _value)
{
	if (!_name)
		return false;

	HashString name;
	HashString value;
	if (!name.SetTo(_name) || !value.SetTo(_value))
		return false;
	
	return (Put(name, value) == B_OK);
}

/*
 * Copyright 2002-2007, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StringTokenizer.h"


const char* const StringTokenizer::kDefaultSeparators = " \t\n\r";


// constructor
StringTokenizer::StringTokenizer(const char* string, const char* separators,
		const char* trimChars)
	: fString(string),
	  fSeparators(separators),
	  fTrimChars(trimChars),
	  fStringLength(strlen(string)),
	  fIndex(0)
{
}


// NextToken
bool
StringTokenizer::NextToken(BString& token, const char* separators,
	const char* trimChars)
{
	if (!separators)
		separators = (fSeparators != NULL ? fSeparators : kDefaultSeparators);

	if (!trimChars)
		trimChars = fTrimChars;

	// skip separators
	while (fIndex < fStringLength
		&& strchr(separators, fString[fIndex]) != NULL) {
		fIndex++;
	}

	int32 startIndex = fIndex;

	// skip non-separators
	while (fIndex < fStringLength
		&& strchr(separators, fString[fIndex]) == NULL) {
		fIndex++;
	}

	if (startIndex == fIndex)
		return false;

	int32 endIndex = fIndex;

	// trim, if requested
	if (trimChars != NULL) {
		while (startIndex < endIndex
			&& strchr(trimChars, fString[startIndex]) != NULL) {
			startIndex++;
		}

		while (startIndex < endIndex
			&& strchr(trimChars, fString[endIndex - 1]) != NULL) {
			endIndex--;
		}
	}

	token.SetTo(fString + startIndex, endIndex - startIndex);

	return true;
}

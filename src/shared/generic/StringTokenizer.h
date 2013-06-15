/*
 * Copyright 2002-2007, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef STRING_TOKENIZER_H
#define STRING_TOKENIZER_H

#include <String.h>


class StringTokenizer {
public:
								StringTokenizer(const char* string,
									const char* separators = NULL,
									const char* trimChars = NULL);

			bool				NextToken(BString& token,
									const char* separators = NULL,
									const char* trimChars = NULL);

public:
	static	const char*	const	kDefaultSeparators;

private:
			const char*			fString;
			const char*			fSeparators;
			const char*			fTrimChars;
			int32				fStringLength;
			int32				fIndex;
};



#endif	// STRING_TOKENIZER_H

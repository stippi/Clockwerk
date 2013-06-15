/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef ARGUMENT_VECTOR_H
#define ARGUMENT_VECTOR_H

#include <List.h>

class BString;

class ArgumentVector {
public:
								ArgumentVector();
								~ArgumentVector();

			const char* const*	Arguments() const;
			int32				ArgumentCount() const;

			status_t			Parse(const char* args,
									BString* errorMessage = NULL);
			void				Clear();

private:
			class ArgvParser;
			friend class ArgvParser;

			status_t			_AppendArg(const char* arg);

			BList				fArguments;
};

#endif	// ARGUMENT_VECTOR_H

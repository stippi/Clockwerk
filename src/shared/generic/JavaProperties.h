/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef JAVA_PROPERTIES_H
#define JAVA_PROPERTIES_H


#include "HashMap.h"
#include "HashString.h"


class BDataIO;


class JavaProperties : public HashMap<HashString, HashString> {
public:
								JavaProperties();
								JavaProperties(JavaProperties* parent);
								~JavaProperties();

			status_t			Load(const char* fileName);
			status_t			Load(BDataIO& input);

			status_t			Save(const char* fileName);
			status_t			Save(BDataIO& output);

			const char*			GetProperty(const char* name,
									const char* defaultValue = NULL) const;
			bool				SetProperty(const char* name,
									const char* value);

private:
			class Parser;

			JavaProperties*		fParent;
};



#endif	// JAVA_PROPERTIES_H

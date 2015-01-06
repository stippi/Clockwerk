/*
 * Copyright 2002-2007, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef HASH_STRING_H
#define HASH_STRING_H

#include <SupportDefs.h>

#ifndef CLOCKWERK_FOR_HAIKU

// strnlen
static inline
size_t
strnlen(const char *str, size_t maxLen)
{
	if (str) {
		size_t origMaxLen = maxLen;
		while (maxLen > 0 && *str != '\0') {
			maxLen--;
			str++;
		}
		return origMaxLen - maxLen;
	}
	return 0;
}


#ifndef CLOCKWERK_FOR_ZETA

// strlcpy
static inline size_t
strlcpy(char *dst, char const *src, size_t s)
{
	size_t i= 0;

	if (!s)
		return strlen(src);

	for (i = 0; ((i < s - 1) && src[i]); i++) {
		dst[i] = src[i];
	}

	dst[i] = 0;

	return i + strlen(src + i);
}

#endif	// CLOCKWERK_FOR_ZETA
#endif	// CLOCKWERK_FOR_HAIKU

// string_hash
//
// from the Dragon Book: a slightly modified hashpjw()
static inline
uint32
string_hash(const char *name)
{
	uint32 h = 0;
	if (name) {
		for (; *name; name++) {
			uint32 g = h & 0xf0000000;
			if (g)
				h ^= g >> 24;
			h = (h << 4) + *name;
		}
	}
	return h;
}


#ifdef __cplusplus


// HashString
class HashString {
public:
	inline						HashString();
	inline						HashString(const HashString& other);
	inline						HashString(const char* string, int32 length = -1);
	inline						~HashString();

			bool				SetTo(const char* string, int32 maxLength = -1);
	inline	void				Unset()				{ SetTo(NULL, 0); }

			bool				Truncate(int32 newLength);

	inline	const char*			GetString() const	{ return fData->data; }
	inline	int32				GetLength() const	{ return fLength; }

	inline	uint32				GetHashCode() const
									{ return string_hash(GetString()); }

	inline	HashString&				operator=(const HashString& other);
	inline	HashString&				operator=(const char* other);
			bool				operator==(const HashString& other) const;
			bool				operator==(const char* other) const;
	inline	bool				operator!=(const HashString& other) const
									{ return !(*this == other); }
	inline	bool				operator!=(const char* other) const
									{ return !(*this == other); }

private:
	struct Data {
		int32	refCount;
		char	data[1];
	};

			void				_Unset();

private:
	static	Data				sEmptyString;

			Data*				fData;
			int32				fLength;
};


// constructor
inline
HashString::HashString()
	: fData(&sEmptyString),
	  fLength(0)
{
	atomic_add(&fData->refCount, 1);
}


// copy constructor
inline
HashString::HashString(const HashString& other)
	: fData(other.fData),
	  fLength(other.fLength)
{
	atomic_add(&fData->refCount, 1);
}


// constructor
inline
HashString::HashString(const char* string, int32 length)
	: fData(NULL),
	  fLength(0)
{
	SetTo(string, length);
}


// destructor
inline
HashString::~HashString()
{
	_Unset();
}


// =
HashString &
HashString::operator=(const HashString& other)
{
	if (&other != this) {
		fData = other.fData;
		fLength = other.fLength;
		atomic_add(&fData->refCount, 1);
	}

	return *this;
}


// =
HashString &
HashString::operator=(const char* other)
{
	SetTo(other);
	return *this;
}


#endif	// __cplusplus

#endif	// HASH_STRING_H

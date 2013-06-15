/*
 * Copyright 2007, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SHA_256_H
#define SHA_256_H


#include <String.h>


class SHA256 {
public:
								SHA256();
								~SHA256();

			void				Update(const void* buffer, size_t size);
			const uint8*		Digest();
			size_t				DigestLength() const	{ return 32; }

			void				GetDigestAsString(BString& string);

private:
			void				_ProcessChunk();

private:
			uint32				fHash[8];
			uint32				fDigest[8];
			uint32				fBuffer[64];
			size_t				fBytesInBuffer;
			size_t				fMessageSize;
			bool				fDigested;
};


#endif	// SHA_256_H

/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <DataIO.h>

#include "BinDataIOInputStream.h"

// constructor
BinDataIOInputStream::BinDataIOInputStream(BDataIO& input)
	: BinInputStream(),
	  fInput(input),
	  fPosition(0)
{
}

// destructor
BinDataIOInputStream::~BinDataIOInputStream()
{
}

// curPos
unsigned int
BinDataIOInputStream::curPos() const
{
	return fPosition;
}

// readBytes
unsigned int
BinDataIOInputStream::readBytes(XMLByte* toFill, const unsigned int maxToRead)
{
	unsigned int read = 0;
	if (toFill) {
		ssize_t bytesRead = fInput.Read(toFill, maxToRead);
		if (bytesRead >= 0)
			read = bytesRead;
	}
	fPosition += read;
	return read;
}


/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "DataIOInputSource.h"
#include "BinDataIOInputStream.h"

// constructor
DataIOInputSource::DataIOInputSource(BDataIO* input)
	: InputSource(),
	  fInput(input)
{
}

// destructor
DataIOInputSource::~DataIOInputSource()
{
}

// makeStream
BinInputStream*
DataIOInputSource::makeStream() const
{
	return new BinDataIOInputStream(*fInput);
}

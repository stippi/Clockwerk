/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DATA_IO_INPUT_SOURCE_H
#define DATA_IO_INPUT_SOURCE_H

#include <InputSource.hpp>

class BDataIO;

class DataIOInputSource : public InputSource {
 public:
								DataIOInputSource(BDataIO* input);
	virtual						~DataIOInputSource();

	virtual	BinInputStream*		makeStream() const;

 private:
	mutable	BDataIO*			fInput;
};

#endif	// DATA_IO_INPUT_SOURCE_H

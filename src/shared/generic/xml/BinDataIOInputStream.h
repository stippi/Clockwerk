/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef BIN_DATA_IO_INPUT_STREAM_H
#define BIN_DATA_IO_INPUT_STREAM_H

#include <BinInputStream.hpp>

class BDataIO;

class BinDataIOInputStream : public BinInputStream {
 public:
								BinDataIOInputStream(BDataIO& input);
	virtual						~BinDataIOInputStream();

	virtual	unsigned int		curPos() const;
	virtual	unsigned int		readBytes(XMLByte* toFill,
										  const unsigned int maxToRead);

 private:
			BDataIO&			fInput;
			unsigned int		fPosition;
};

#endif	// BIN_DATA_IO_INPUT_STREAM_H

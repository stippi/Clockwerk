/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef IMPORTER_H
#define IMPORTER_H

#include <Entry.h>

class BPositionIO;
class ServerObjectManager;
class Playlist;

class Importer {
 public:
								Importer();
	virtual						~Importer();

	virtual	status_t			Import(Playlist* playlist, BPositionIO* stream,
									const entry_ref* refToOriginalFile) = 0;
};


#endif // IMPORTER_H

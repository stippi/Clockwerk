/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EXPORTER_H
#define EXPORTER_H

#include <Entry.h>

class BPositionIO;
class Playlist;
//class ExportPanel;

class Exporter {
 public:
								Exporter();
	virtual						~Exporter();

	virtual	status_t			Export(Playlist* playlist,
									   BPositionIO* stream,
									   const entry_ref* refToFinalFile) = 0;

	virtual	const char*			MIMEType() const = 0;
	virtual	const char*			Extension() const = 0;

//	virtual	void				CustomizeExportPanel(ExportPanel* panel) = 0;
//
//	static	Exporter*			ExporterFor(uint32 mode);
};


#endif // EXPORTER_H

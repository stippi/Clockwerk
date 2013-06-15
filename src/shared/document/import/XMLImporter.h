/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_IMPORTER_H
#define XML_IMPORTER_H


#include "HashMap.h"
#include "HashString.h"
#include "Importer.h"


class Clip;
class ClipPlaylistItem;
class FileBasedClip;
class PlaylistItem;
class PropertyObject;
class ScrollingTextClip;
class XMLHelper;
class XMLStorable;

class XMLImporter : public Importer {
 public:
								XMLImporter();
	virtual						~XMLImporter();

	// Importer interface
	virtual	status_t			Import(Playlist* playlist, BPositionIO* stream,
									const entry_ref* refToOriginalFile);

	// XMLImporter
			status_t			Restore(XMLStorable* object,
									BPositionIO* stream);

 private:
			status_t			_RestoreDocument(XMLHelper& xml,
									Playlist* playlist);

			status_t			_BuildClipIdIndexMap(XMLHelper& xml);

			status_t			_RestorePlaylist(XMLHelper& xml,
									Playlist* playlist);

	typedef HashMap<HashKey32<int32>, HashString> IndexClipIdMap;
			IndexClipIdMap		fIndexClipIdMap;
};

#endif // XML_IMPORTER_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_EXPORTER_H
#define XML_EXPORTER_H


#include "Exporter.h"
#include "HashMap.h"
#include "HashString.h"


class BString;
class Clip;
class ClipPlaylistItem;
class Playlist;
class PlaylistItem;
class PropertyObject;
class ServerObjectManager;
class TrackProperties;
class XMLHelper;
class XMLStorable;

class XMLExporter : public Exporter {
 public:
								XMLExporter();
	virtual						~XMLExporter();

	// Exporter interface
	virtual	status_t			Export(Playlist* playlist,
									   BPositionIO* stream,
									   const entry_ref* refToFinalFile);

	virtual	const char*			MIMEType() const;
	virtual	const char*			Extension() const;

	// XMLExporter
			status_t			Store(const XMLStorable* object,
									  BPositionIO* stream);

//	virtual	void				CustomizeExportPanel(ExportPanel* panel) = 0;
 private:
			status_t			_StoreClipLibrary(XMLHelper& xml,
												  const Playlist* list);
			status_t			_StoreAllClips(XMLHelper& xml,
											   const Playlist* list);
			status_t			_StoreClipID(XMLHelper& xml,
									const BString& clipID, int32 index,
									const BString& templateName,
									int32 version);

			status_t			_StorePlaylist(XMLHelper& xml,
											   Playlist* list);
			status_t			_StorePlaylistItem(XMLHelper& xml,
												   PlaylistItem* item);
			status_t			_StorePlaylistItem(XMLHelper& xml,
												   ClipPlaylistItem* item);
			status_t			_StoreTrackProperties(XMLHelper& xml,
												   TrackProperties* properties);

	typedef HashMap<HashString, int32> ClipIdIndexMap;
			ClipIdIndexMap		fClipIdIndexMap;
			int32				fLastClipIdIndex;
};


#endif // XML_EXPORTER_H

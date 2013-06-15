/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "XMLImporter.h"

#include <new>

#include <ByteOrder.h>
#include <DataIO.h>
#include <Path.h>

#include "common.h"
#include "xml_import.h"

#include "Clip.h"
#include "ClipPlaylistItem.h"
#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "NavigationInfo.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "PropertyObjectFactory.h"
#include "ServerObject.h"
#include "TrackProperties.h"
#include "XMLHelper.h"
#include "XMLSupport.h"

using std::nothrow;

// constructor
XMLImporter::XMLImporter()
	: Importer()
{
}

// destructor
XMLImporter::~XMLImporter()
{
}

// Import
status_t
XMLImporter::Import(Playlist* playlist, BPositionIO* stream,
	const entry_ref* refToFinalFile)
{
	fIndexClipIdMap.Clear();

	XMLHelper* xmlHelper = create_xml_helper();

	XMLHelper& xml = *xmlHelper;
	if (!xml.Lock()) {
		delete xmlHelper;
		return B_ERROR;
	}

	status_t ret = xml.Load(*stream);

	if (ret == B_OK)
		ret = _RestoreDocument(xml, playlist);

	xml.Unlock();
	delete xmlHelper;

	return ret;
}

// Restore
status_t
XMLImporter::Restore(XMLStorable* object, BPositionIO* stream)
{
	XMLHelper* xmlHelper = create_xml_helper();

	XMLHelper& xml = *xmlHelper;
	if (!xml.Lock()) {
		delete xmlHelper;
		return B_ERROR;
	}

	status_t ret = xml.Load(*stream);

	if (ret == B_OK)
		ret = xml.RestoreObject(object);

	xml.Unlock();
	delete xmlHelper;

	return ret;
}

// #pragma mark -

// _RestoreDocument
status_t
XMLImporter::_RestoreDocument(XMLHelper& xml, Playlist* playlist)
{
	BString t;
	xml.GetTagName(t);
	if (t != "CLOCKWERK")
		return B_BAD_VALUE;

	status_t ret = xml.OpenTag("REFERENCED_OBJECTS");

	if (ret == B_OK) {
		ret = _BuildClipIdIndexMap(xml);
		xml.CloseTag(); // REFERENCED_OBJECTS
	}

	ret = xml.OpenTag("PLAYLIST");

	if (ret == B_OK) {
		// restore items
		ret = _RestorePlaylist(xml, playlist);
		xml.CloseTag(); // PLAYLIST
	}

	return ret;
}

// _BuildClipIdIndexMap
status_t
XMLImporter::_BuildClipIdIndexMap(XMLHelper& xml)
{
	status_t ret = B_OK;

	while (xml.OpenTag("OBJECT") == B_OK) {
		BString clipID = xml.GetAttribute("id", "");
		if (clipID.Length() <= 0)
			ret = B_ERROR;

		int32 index = xml.GetAttribute("index", (int32)-1);
		if (index >= 0) {
			if (fIndexClipIdMap.ContainsKey(index)) {
				print_error("XMLImporter::_BuildClipIdIndexMap() - "
					"inconsistent REFERENCED_OBJECTS tag!\n");
			} else {
				ret = fIndexClipIdMap.Put(index, clipID.String());
			}
		} // else this is an old playlist file

		if (ret == B_OK)
			ret = xml.CloseTag(); // OBJECT

		if (ret != B_OK)
			break;
	}
	return ret;
}

// _RestorePlaylist
status_t
XMLImporter::_RestorePlaylist(XMLHelper& xml, Playlist* list)
{
	status_t ret = B_OK;

	PlaylistNotificationBlock _(list);

	while (xml.OpenTag("ITEM") == B_OK) {

		BString clipID;
		int32 clipIdIndex = xml.GetAttribute("clip_index", (int32)-1);
		if (clipIdIndex >= 0 && fIndexClipIdMap.ContainsKey(clipIdIndex)) {
			// the clip id was properly read from the referenced object section
			clipID = fIndexClipIdMap.Get(clipIdIndex).GetString();
		} else {
			// try to fall back to old storage format
			clipID = xml.GetAttribute("clip_id", "");
		}
		ClipPlaylistItem* item = new (nothrow) ClipPlaylistItem((Clip*)NULL);
		if (!item)
			ret = B_NO_MEMORY;
		if (ret == B_OK) {
			// remember the clip id in a temporary property that
			// is removed when dependencies are resolved
			item->AddProperty(new (nothrow) StringProperty(PROPERTY_CLIP_ID,
				clipID.String()));
		}

		if (item) {
			// restore track
			uint32 track = xml.GetAttribute("track", (uint32)0);
			item->SetTrack(track);
			// restore clip offset
			uint64 clipOffset = xml.GetAttribute("clip_offset", (uint64)0);
			item->SetClipOffset(clipOffset);
			// restore startframe
			int64 startFrame = xml.GetAttribute("startframe", (int64)0);
			item->SetStartFrame(startFrame);
			// restore duration
			uint64 duration = xml.GetAttribute("duration", (uint64)0);
			if (duration > 0)
				item->SetDuration(duration);

			// restore video/audio muted
			item->SetVideoMuted(xml.GetAttribute("video_muted", false));
			item->SetAudioMuted(xml.GetAttribute("audio_muted", false));

			// restore item properties
			ret = restore_properties(xml, item);

			// restore navigation info if it exists
			if (ret == B_OK && xml.OpenTag("NAV_INFO") == B_OK) {
				NavigationInfo info;
				ret = info.XMLRestore(xml);
				if (ret == B_OK)
					item->SetNavigationInfo(&info);
				if (ret == B_OK)
					ret = xml.CloseTag(); // NAV_INFO
			}

			if (ret != B_OK) {
				delete item;
				break;
			}
				
			if (!list->AddItem(item)) {
				ret = B_NO_MEMORY;
				delete item;
			}
		}

		if (ret == B_OK)
			ret = xml.CloseTag(); // ITEM

		if (ret != B_OK)
			break;
	}

	// restore track properties
	while (xml.OpenTag("TRACK_PROPERTIES") == B_OK) {

		TrackProperties properties(0);
		ret = properties.XMLRestore(xml);

		if (ret == B_OK && !list->SetTrackProperties(properties))
			ret = B_NO_MEMORY;

		if (ret == B_OK)
			ret = xml.CloseTag();
		
		if (ret != B_OK)
			break;
	}

	// restore solo track
	if (ret == B_OK)
		list->SetSoloTrack(xml.GetAttribute("solo_track", (int32)-1));

	return ret;
}


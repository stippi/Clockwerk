/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "XMLExporter.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <Path.h>

#include "xml_export.h"

#include "Clip.h"
#include "ClipPlaylistItem.h"
#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "NavigationInfo.h"
#include "Playlist.h"
#include "PlaylistItem.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "ServerObjectManager.h"
#include "TrackProperties.h"
#include "XMLHelper.h"
#include "XMLSupport.h"

// constructor
XMLExporter::XMLExporter()
	: Exporter()
{
}

// destructor
XMLExporter::~XMLExporter()
{
}

// Export
status_t
XMLExporter::Export(Playlist* playlist, BPositionIO* stream,
					const entry_ref* refToFinalFile)
{
	fClipIdIndexMap.Clear();
	fLastClipIdIndex = 0;

	XMLHelper* xmlHelper = create_xml_helper();

	XMLHelper& xml = *xmlHelper;
	if (!xml.Lock()) {
		delete xmlHelper;
		return B_ERROR;
	}

	xml.Init("CLOCKWERK");

//	BPath path(refToFinalFile);
//	status_t ret = path.InitCheck();
//	if (ret == B_OK) {
//		// NOTE: this is a handy information in case a
//		// documents true path is different from the
//		// "path" attribute when it is loaded later on
//		ret = xml.SetAttribute("path", path.Path());
//	}

	// write the clip library
	status_t ret = _StoreClipLibrary(xml, playlist);

	// write the playlist
	if (ret == B_OK)
		ret = _StorePlaylist(xml, playlist);

	// save the file
	if (ret == B_OK)
		ret = xml.Save(*stream);

	xml.Unlock();
	delete xmlHelper;

	return ret;
}

// MIMEType
const char*
XMLExporter::MIMEType() const
{
	return "text/x-clockwerk";
}

// Extension
const char*
XMLExporter::Extension() const
{
	return "cwp";
}

// #pragma mark -

// Store
status_t
XMLExporter::Store(const XMLStorable* object, BPositionIO* stream)
{
	XMLHelper* xmlHelper = create_xml_helper();

	XMLHelper& xml = *xmlHelper;
	if (!xml.Lock()) {
		delete xmlHelper;
		return B_ERROR;
	}

	xml.Init("CLOCKWERK_OBJECT");

	// store the object
	status_t ret = xml.StoreObject(object);

	// save the file
	if (ret == B_OK)
		ret = xml.Save(*stream);

	xml.Unlock();
	delete xmlHelper;

	return ret;
}

// #pragma mark -

// _StoreClipLibrary
status_t
XMLExporter::_StoreClipLibrary(XMLHelper& xml, const Playlist* list)
{
	status_t ret = xml.CreateTag("REFERENCED_OBJECTS");

	if (ret == B_OK)
		ret = _StoreAllClips(xml, list);

	if (ret == B_OK)
		ret = xml.CloseTag(); // REFERENCED_OBJECTS

	return ret;
}

// _StoreAllClips
status_t
XMLExporter::_StoreAllClips(XMLHelper& xml, const Playlist* list)
{
	status_t ret = B_OK;

	int32 count = list->CountItems(); 
	for (int32 i = 0; i < count; i++) {
		ClipPlaylistItem* ci = dynamic_cast<ClipPlaylistItem*>(list->ItemAtFast(i));
		if (ci) {
			// just store the id
			BString clipID;
			BString templateName = "";
			int32 version = -1;
			if (ci->Clip()) {
				clipID = ci->Clip()->ID();
				version = ci->Clip()->Version();
				if (ci->Clip()->IsTemplate())
					templateName = ci->Clip()->TemplateName();
			} else {
				// the item still knows the id of it's clip,
				// but the dependencies was not resolved,
				// in case the clip was a template, it will
				// not be remembered in the playlist anymore
				ci->GetValue(PROPERTY_CLIP_ID, clipID);
			}
			
			if (!fClipIdIndexMap.ContainsKey(clipID.String())) {
				// we didn't encounter this clip before, give it
				// a new index
				ret = fClipIdIndexMap.Put(clipID.String(), fLastClipIdIndex);
				if (ret == B_OK) {
					int32 index = fLastClipIdIndex;
					fLastClipIdIndex++;
	
					ret = _StoreClipID(xml, clipID, index,
						templateName, version);
				}
			} // else we already stored this clip
		}

		if (ret != B_OK)
			break;
	}

	return ret;
}

// _StoreClipID
status_t
XMLExporter::_StoreClipID(XMLHelper& xml, const BString& clipID, int32 index,
	const BString& templateName, int32 version)
{
	status_t ret = xml.CreateTag("OBJECT");

	if (ret == B_OK)
		ret = xml.SetAttribute("id", clipID.String());

	if (ret == B_OK)
		ret = xml.SetAttribute("index", index);

	if (ret == B_OK && version >= 0)
		ret = xml.SetAttribute("version", version);

	if (ret == B_OK && templateName.Length() > 0)
		ret = xml.SetAttribute("template_name", templateName.String());

	if (ret == B_OK)
		ret = xml.CloseTag();

	return ret;
}

// #pragma mark -

// _StorePlaylist
status_t
XMLExporter::_StorePlaylist(XMLHelper& xml, Playlist* list)
{
	status_t ret = xml.CreateTag("PLAYLIST");

	if (ret == B_OK) {
		int32 count = list->CountItems(); 
		for (int32 i = 0; i < count; i++) {
			PlaylistItem* item = list->ItemAtFast(i);
			ret = _StorePlaylistItem(xml, item);
			if (ret != B_OK)
				break;
		}
	}

	if (ret == B_OK) {
		int32 count = list->CountTrackProperties(); 
		for (int32 i = 0; i < count; i++) {
			TrackProperties* properties = list->TrackPropertiesAtFast(i);
			ret = _StoreTrackProperties(xml, properties);
			if (ret != B_OK)
				break;
		}
	}

	if (ret == B_OK && list->SoloTrack() >= 0)
		ret = xml.SetAttribute("solo_track", list->SoloTrack());

	if (ret == B_OK)
		ret = xml.CloseTag();

	return ret;
}

// _StorePlaylistItem
status_t
XMLExporter::_StorePlaylistItem(XMLHelper& xml, PlaylistItem* item)
{
	if (ClipPlaylistItem* ci = dynamic_cast<ClipPlaylistItem*>(item)) {
		return _StorePlaylistItem(xml, ci);
	}
	// unkown item type
printf("XMLExporter::_StorePlaylistItem() - unkown item type!\n");
	return B_ERROR;
}

// _StorePlaylistItem
status_t
XMLExporter::_StorePlaylistItem(XMLHelper& xml, ClipPlaylistItem* item)
{
	status_t ret = xml.CreateTag("ITEM");

	if (ret == B_OK && item->StartFrame() != 0)
		ret = xml.SetAttribute("startframe", item->StartFrame());

	if (ret == B_OK)
		ret = xml.SetAttribute("duration", item->Duration());

	if (ret == B_OK && item->Track() != 0)
		ret = xml.SetAttribute("track", item->Track());

	if (ret == B_OK)
		ret = xml.SetAttribute("clip_offset", item->ClipOffset());

	if (ret == B_OK && item->IsVideoMuted())
		ret = xml.SetAttribute("video_muted", true);

	if (ret == B_OK && item->IsAudioMuted())
		ret = xml.SetAttribute("audio_muted", true);

	if (ret == B_OK) {
		BString clipID;
		if (item->Clip()) {
			clipID = item->Clip()->ID();
		} else {
			item->GetValue(PROPERTY_CLIP_ID, clipID);
		}
		if (fClipIdIndexMap.ContainsKey(clipID.String())) {
			// the map should always contain the id!
			ret = xml.SetAttribute("clip_index",
				fClipIdIndexMap.Get(clipID.String()));
		} else {
			// this is a fallback, but we should not be here!
			ret = xml.SetAttribute("clip_id", clipID.String());
		}
	}

	if (ret == B_OK)
		ret = store_properties(xml, item);

	// store the NavigationInfo if it exists
	if (const NavigationInfo* info = item->NavigationInfo()) {
		ret = xml.CreateTag("NAV_INFO");
		if (ret == B_OK)
			ret = info->XMLStore(xml);
		if (ret == B_OK)
			ret = xml.CloseTag();
	}

	if (ret == B_OK)
		ret = xml.CloseTag();

	return ret;
}

// _StoreTrackProperties
status_t
XMLExporter::_StoreTrackProperties(XMLHelper& xml, TrackProperties* properties)
{
	status_t ret = xml.CreateTag("TRACK_PROPERTIES");

	if (ret == B_OK)
		ret = properties->XMLStore(xml);

	if (ret == B_OK)
		ret = xml.CloseTag();

	return ret;
}

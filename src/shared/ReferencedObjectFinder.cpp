/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ReferencedObjectFinder.h"

#include <new>

#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <String.h>

#include "common.h"

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "CommonPropertyIDs.h"
#include "Logger.h"
#include "PropertyObject.h"
#include "RequestXMLConverter.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "support.h"
#include "XMLHelper.h"
#include "XMLSupport.h"

using std::nothrow;


static Logger sLog("ReferencedObjectFinder");


// FindReferencedObjects
status_t
ReferencedObjectFinder::FindReferencedObjects(ServerObjectManager* library,
	const ServerObject* object, IdSet& foundObjects)
{
	if (!object)
		return B_BAD_VALUE;

	BString type(object->Type());
	if (type == "CollectingPlaylist") {
		// parse Playlist file
		return _GetPlaylistOrScheduleClips(library, object, foundObjects,
			false, true);
	} else if (string_ends_with(type, "Playlist")) {
		// parse Playlist file
		return _GetPlaylistOrScheduleClips(library, object, foundObjects,
			false);
	} else if (type == "Schedule") {
		// parse Schedule file
		return _GetPlaylistOrScheduleClips(library, object, foundObjects,
			true);
	} else if (type == "ClockwerkRevision") {
		// parse Revision file
		return _GetRevisionObjects(library, object->ID(), foundObjects);
// NOTE: for the time being, ClientSettings don't reference anything
//	} else if (type == "ClientSettings") {
//		// find referenced playlist id
//		BString referencedID;
//		if (object->GetValue(PROPERTY_REFERENCED_PLAYLIST, referencedID))
//			return foundObjects.Add(referencedID.String());
//		else
//			return B_ERROR;
//		// TODO: display settings (PROPERTY_REFERENCED_DISPLAY_SETTINGS)
	} else {
		LOG_DEBUG("FindReferencedObjects(): no references for type %s\n",
			type.String());
	}

	return B_OK;
}

// #pragma mark -

// _GetPlaylistOrScheduleClips
status_t
ReferencedObjectFinder::_GetPlaylistOrScheduleClips(ServerObjectManager* library,
	const ServerObject* object, IdSet& foundObjects, bool schedule,
	bool ignoreReferenced)
{
	// locate playlist file
	entry_ref ref;
	library->GetRef(object->ID(), ref);
	
	BFile file(&ref, B_READ_ONLY);
	status_t ret = file.InitCheck();
	if (ret < B_OK) {
		LOG_ERROR("_GetPlaylistOrScheduleClips(): failed to open file for "
			"object %s\n", object->ID().String());
		return ret;
	}

	// actually parse the file
	XMLHelper* xml = create_xml_helper();
	if (!xml)
		return B_NO_MEMORY;

	ObjectDeleter<XMLHelper> deleter(xml);

	AutoLocker<XMLHelper> locker(xml);
	if (!locker.IsLocked())
		return B_ERROR;

	ret = xml->Load(file);
	if (ret < B_OK) {
		LOG_ERROR("_GetPlaylistOrScheduleClips(): failed to load XML file for "
			"object %s\n", object->ID().String());
		return ret;
	}

	if (schedule) {
		// the schedule is embedded in an additional SCHEDULE tag,
		// otherwise it is structured like a playlist file with
		// regards to referenced/dependency objects
		ret = xml->OpenTag("SCHEDULE");
		if (ret < B_OK)
			return ret;
	}

	if (!ignoreReferenced) {
		// NOTE: for CollectingPlaylists, we need to ignore
		// the "referenced objects", since these only
		// reflect what objects this playlist collected at the
		// time on the editor which saved it, but the whole point
		// of CollectingPlaylists is to collect its objects live.
		ret = xml->OpenTag("REFERENCED_OBJECTS");
		if (ret < B_OK)
			return ret;
	
		while (xml->OpenTag("OBJECT") == B_OK) {
			BString id = xml->GetAttribute("id", "");
			xml->CloseTag();
			if (id.Length() == 0) {
				printf("failed to read id attribute for OBJECT in %s\n",
					object->ID().String());
				LOG_ERROR("_GetPlaylistOrScheduleClips(): failed to read id "
					"attribute for OBJECT in %s\n", object->ID().String());
				continue;
			}
			LOG_DEBUG("_GetPlaylistOrScheduleClips(): object \"%s\" references "
				"object \"%s\"\n", object->ID().String(), id.String());
			ret = foundObjects.Add(id.String());
			if (ret < B_OK)
				break;
		}
	
		if (ret == B_OK)
			xml->CloseTag(); // REFERENCED_OBJECTS
	}

	if (schedule && ret == B_OK) {
		xml->CloseTag(); // SCHEDULE
	}

	if (ret == B_OK) {
		// referenced clip, used by CollectingPlaylist for
		// the clip used as transition between items
		BString referencedClip = object->Value(PROPERTY_CLIP_ID, "");
		if (referencedClip.Length() > 0) {
			LOG_DEBUG("_GetPlaylistOrScheduleClips(): object \"%s\" references "
				"clip \"%s\"\n", object->ID().String(),
				referencedClip.String());
			ret = foundObjects.Add(referencedClip.String());
		}
	}

	if (ret == B_OK) {
		// background sound clip, used by CollectingPlaylist
		BString bgSoundClip = object->Value(PROPERTY_BACKGROUND_SOUND_ID, "");
		if (bgSoundClip.Length() > 0) {
			LOG_DEBUG("_GetPlaylistOrScheduleClips(): object \"%s\" references "
				"background sound \"%s\"\n", object->ID().String(),
				bgSoundClip.String());
			ret = foundObjects.Add(bgSoundClip.String());
		}
	}

	return ret;
}

// _GetRevisionObjects
status_t
ReferencedObjectFinder::_GetRevisionObjects(ServerObjectManager* library,
	BString serverID, IdSet& foundObjects)
{
	entry_ref ref;
	status_t ret = library->GetRef(serverID, ref);
	if (ret < B_OK)
		return ret;

	BFile file(&ref, B_READ_ONLY);
	BMessage revision;
	RequestXMLConverter converter;

	ret = converter.ConvertFromXML(file, &revision);
	if (ret < B_OK) {
		printf("ReferencedObjectFinder::_GetRevisionObjects() - "
			   "converting to BMessage failed: %s\n", strerror(ret));
	}

	BString id;
	for (int32 i = 0; revision.FindString("soid", i, &id) == B_OK; i++) {
		LOG_DEBUG("_GetRevisionObjects(): object \"%s\" references "
			"object \"%s\"\n", serverID.String(), id.String());
		ret = foundObjects.Add(id.String());
		if (ret < B_OK)
			break;
	}

	return ret;
}


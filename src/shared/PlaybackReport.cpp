/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaybackReport.h"

#include <String.h>

#include "CommonPropertyIDs.h"
#include "XMLHelper.h"

// constructor
PlaybackReport::PlaybackReport()
	: ServerObject("PlaybackReport"),
	  XMLStorable()
{
}

// destructor
PlaybackReport::~PlaybackReport()
{
}

// #pragma mark -

// IsMetaDataOnly
bool
PlaybackReport::IsMetaDataOnly() const
{
	return false;
}

// #pragma mark -

// XMLStore
status_t
PlaybackReport::XMLStore(XMLHelper& xml) const
{
	status_t ret = B_OK;

	PlaybackMap::Iterator iterator = fIDPlaybackCountMap.GetIterator();
	while (iterator.HasNext()) {
		PlaybackMap::Entry entry = iterator.Next();

		ret = xml.CreateTag("CLIP");
		if (ret == B_OK)
			ret = xml.SetAttribute("clip_id", entry.key.GetString());
		if (ret == B_OK)
			ret = xml.SetAttribute("playback_count", entry.value);
		if (ret == B_OK)
			ret = xml.CloseTag(); // CLIP

		if (ret < B_OK)
			break;
	}

	return ret;
}

// XMLRestore
status_t
PlaybackReport::XMLRestore(XMLHelper& xml)
{
	status_t ret = B_OK;

	while (xml.OpenTag("CLIP")) {
		BString clipID = xml.GetAttribute("clip_id", "");
		int32 playbackCount = xml.GetAttribute("playback_count", (int32)-1);

		if (clipID.Length() > 0 && playbackCount >= 0) {
			ret = fIDPlaybackCountMap.Put(clipID.String(), playbackCount);
		}

		if (ret == B_OK)
			ret = xml.CloseTag(); // CLIP

		if (ret < B_OK)
			break;
	}

	return ret;
}

// #pragma mark -

// SetUnitID
void
PlaybackReport::SetUnitID(const BString& unitID)
{
	SetValue(PROPERTY_UNIT_ID, unitID.String());
}

// AddReport
bool
PlaybackReport::AddReport(const BString& clipID, uint32 timeOfPlayback)
{
	// NOTE: the current implementation just stores the number of
	// times a clip has been played, "timeOfPlayback" is ignored

	int32 playbackCount = 0;

	// look up old playback count
	if (fIDPlaybackCountMap.ContainsKey(clipID.String()))
		playbackCount = fIDPlaybackCountMap.Get(clipID.String());

	// store new playback count
	if (fIDPlaybackCountMap.Put(clipID.String(), playbackCount + 1) < B_OK) {
		printf("PlaybackReport::AddReport() - out of memory\n");
		return false;
	}

	return true;
}


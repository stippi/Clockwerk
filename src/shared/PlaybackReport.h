/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYBACK_REPORT_H
#define PLAYBACK_REPORT_H

#include <Entry.h>

#include "HashMap.h"
#include "HashString.h"
#include "ServerObject.h"
#include "XMLStorable.h"

class PlaybackReport : public ServerObject, public XMLStorable {
 public:
								PlaybackReport();
	virtual						~PlaybackReport();

	// ServerObject interface
	virtual	bool				IsMetaDataOnly() const;

	// XMLStorable interface
	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

	// PlaybackReport
			void				SetUnitID(const BString& unitID);

			bool				AddReport(const BString& clipID,
										  uint32 timeOfPlayback);

 private:

	typedef HashMap<HashString, int32> PlaybackMap;

			PlaybackMap			fIDPlaybackCountMap;
};

#endif // PLAYBACK_REPORT_H

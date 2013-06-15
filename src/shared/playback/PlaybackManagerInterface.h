/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PLAYBACK_MANAGER_INTERFACE_H
#define PLAYBACK_MANAGER_INTERFACE_H

#include <SupportDefs.h>

class PlaybackManagerInterface {
 public:
								PlaybackManagerInterface();
	virtual						~PlaybackManagerInterface();

	virtual	bool				Lock() = 0;
	virtual	status_t			LockWithTimeout(bigtime_t timeout) = 0;
	virtual	void				Unlock() = 0;

	virtual	void				GetPlaylistTimeInterval(
									bigtime_t startTime, bigtime_t& endTime,
									bigtime_t& xStartTime, bigtime_t& xEndTime,
									float& playingSpeed) const = 0;

	virtual	void				SetCurrentAudioTime(bigtime_t time) = 0;

 private:
};

#endif // PLAYBACK_MANAGER_INTERFACE_H

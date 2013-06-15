/*
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

// This class listens to a PlaybackManager
// 
// the hooks are called by PlaybackManager after
// it executed a command, to keep every listener
// informed
// FrameDropped() is something the nodes can call
// and it is passed onto the contollers, so that
// they can respond by displaying some warning

#ifndef PLAYBACK_LISTENER_H
#define PLAYBACK_LISTENER_H

#include <Rect.h>
#include <SupportDefs.h>

class PlaybackListener {
 public:
								PlaybackListener();
	virtual						~PlaybackListener();

	virtual	void				PlayModeChanged(int32 mode);
	virtual	void				LoopModeChanged(int32 mode);
	virtual	void				LoopingEnabledChanged(bool enabled);

	virtual	void				MovieBoundsChanged(BRect bounds);
	virtual	void				FramesPerSecondChanged(float fps);
	virtual	void				SpeedChanged(float speed);

	virtual	void				CurrentFrameChanged(double frame);

	virtual	void				SwitchPlaylistIfNecessary();
	virtual	void				FrameDropped();
};


#endif	// PLAYBACK_LISTENER_H

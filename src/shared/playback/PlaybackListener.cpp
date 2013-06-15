/*
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include "PlaybackListener.h"

// constructor
PlaybackListener::PlaybackListener()
{
}

// destructor
PlaybackListener::~PlaybackListener()
{
}

// PlayModeChanged
void
PlaybackListener::PlayModeChanged(int32 mode)
{
}

// LoopModeChanged
void
PlaybackListener::LoopModeChanged(int32 mode)
{
}

// LoopingEnabledChanged
void
PlaybackListener::LoopingEnabledChanged(bool enabled)
{
}

// MovieBoundsChanged
void
PlaybackListener::MovieBoundsChanged(BRect bounds)
{
}

// FramesPerSecondChanged
void
PlaybackListener::FramesPerSecondChanged(float fps)
{
}

// CurrentFrameChanged
void
PlaybackListener::CurrentFrameChanged(double frame)
{
}

// SpeedChanged
void
PlaybackListener::SpeedChanged(float speed)
{
}

// SwitchPlaylistIfNecessary
void
PlaybackListener::SwitchPlaylistIfNecessary()
{
}

// FrameDropped
void
PlaybackListener::FrameDropped()
{
}

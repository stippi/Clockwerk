/*
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <Message.h>

#include "PlaybackLOAdapter.h"

// constructor
PlaybackLOAdapter::PlaybackLOAdapter(BHandler* handler)
	: AbstractLOAdapter(handler)
{
}

// constructor
PlaybackLOAdapter::PlaybackLOAdapter(const BMessenger& messenger)
	: AbstractLOAdapter(messenger)
{
}

// destructor
PlaybackLOAdapter::~PlaybackLOAdapter()
{
}

// PlayModeChanged
void
PlaybackLOAdapter::PlayModeChanged(int32 mode)
{
	BMessage message(MSG_PLAYBACK_PLAY_MODE_CHANGED);
	message.AddInt32("play mode", mode);
	DeliverMessage(message);
}

// LoopModeChanged
void
PlaybackLOAdapter::LoopModeChanged(int32 mode)
{
	BMessage message(MSG_PLAYBACK_LOOP_MODE_CHANGED);
	message.AddInt32("loop mode", mode);
	DeliverMessage(message);
}

// LoopingEnabledChanged
void
PlaybackLOAdapter::LoopingEnabledChanged(bool enabled)
{
	BMessage message(MSG_PLAYBACK_LOOPING_ENABLED_CHANGED);
	message.AddBool("looping enabled", enabled);
	DeliverMessage(message);
}

// MovieBoundsChanged
void
PlaybackLOAdapter::MovieBoundsChanged(BRect bounds)
{
	BMessage message(MSG_PLAYBACK_MOVIE_BOUNDS_CHANGED);
	message.AddRect("movie bounds", bounds);
	DeliverMessage(message);
}

// FramesPerSecondChanged
void
PlaybackLOAdapter::FramesPerSecondChanged(float fps)
{
	BMessage message(MSG_PLAYBACK_FPS_CHANGED);
	message.AddFloat("fps", fps);
	DeliverMessage(message);
}

// CurrentFrameChanged
void
PlaybackLOAdapter::CurrentFrameChanged(double frame)
{
	BMessage message(MSG_PLAYBACK_CURRENT_FRAME_CHANGED);
	message.AddDouble("current frame", frame);
	DeliverMessage(message);
}

// SpeedChanged
void
PlaybackLOAdapter::SpeedChanged(float speed)
{
	BMessage message(MSG_PLAYBACK_SPEED_CHANGED);
	message.AddFloat("speed", speed);
	DeliverMessage(message);
}

// FrameDropped
void
PlaybackLOAdapter::FrameDropped()
{
	DeliverMessage(MSG_PLAYBACK_FRAME_DROPPED);
}


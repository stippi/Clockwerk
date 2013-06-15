/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaybackController.h"

#include <stdio.h>

#include <Message.h>

#include "CurrentFrame.h"
#include "DisplayRange.h"
#include "LoopMode.h"
#include "PlaybackLOAdapter.h"
#include "PlaybackManager.h"

// constructor
PlaybackController::PlaybackController(BRect frame, CurrentFrame* currentFrame)
	:
	TransportControlGroup(frame, true, true, true, false, false),
	fPlaybackManager(NULL),
	fPlaybackListener(NULL),
	fCurrentFrame(currentFrame),
	fPlaybackRange(NULL),
	fDisplayRange(NULL),
	fLoopMode(NULL),
	fLastCurrentFrame(fCurrentFrame->Frame()),
	fPlayAfterDragging(false)
{
	fCurrentFrame->AddObserver(this);
	SetFlags(Flags() | B_WILL_DRAW);
	float left;
	float top;
	float right;
	float bottom;
	GroupLayout()->GetInsets(&left, &top, &right, &bottom);
	GroupLayout()->SetInsets(left, top + 2, right, bottom);
}

// destructor
PlaybackController::~PlaybackController()
{
	if (fPlaybackManager)
		fPlaybackManager->RemoveListener(fPlaybackListener);
	delete fPlaybackListener;
	fCurrentFrame->RemoveObserver(this);
	if (fPlaybackRange)
		fPlaybackRange->RemoveObserver(this);
	if (fDisplayRange)
		fDisplayRange->RemoveObserver(this);
	if (fLoopMode)
		fLoopMode->RemoveObserver(this);
}

// AttachedToWindow
void
PlaybackController::AttachedToWindow()
{
	TransportControlGroup::AttachedToWindow();

	if (fPlaybackListener)
		return;

	fPlaybackListener = new PlaybackLOAdapter(this);
	if (fPlaybackManager)
		fPlaybackManager->AddListener(fPlaybackListener);
}

// Draw
void
PlaybackController::Draw(BRect updateRect)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	rgb_color lightenMax = tint_color(base, B_LIGHTEN_MAX_TINT);
	rgb_color darken2 = tint_color(base, B_DARKEN_2_TINT);

	BRect r(Bounds());

	BeginLineArray(2);
		AddLine(BPoint(r.left, r.top + 1),
				BPoint(r.right - 1, r.top + 1), lightenMax);
		AddLine(BPoint(r.left, r.top),
				BPoint(r.right, r.top), darken2);
	EndLineArray();
}

// #pragma mark -

// TogglePlaying
void
PlaybackController::TogglePlaying()
{
	if (fPlaybackManager && fPlaybackManager->Lock()) {
		fPlaybackManager->TogglePlaying();
		fPlaybackManager->Unlock();
	}
}

// Stop
void
PlaybackController::Stop()
{
	if (fPlaybackManager && fPlaybackManager->Lock()) {
		if (fPlaybackManager->IsPlaying())
			fPlaybackManager->StopPlaying();
		else {
			if (fLoopMode && fPlaybackRange && fDisplayRange) {
				switch (fLoopMode->Mode()) {
					case LOOPING_RANGE:
						fPlaybackManager->SetCurrentFrame(
							fPlaybackRange->FirstFrame());
						break;
					case LOOPING_VISIBLE:
						fPlaybackManager->SetCurrentFrame(
							fDisplayRange->FirstFrame() + 1);
						break;
					case LOOPING_ALL:
					default:
						fPlaybackManager->SetCurrentFrame(0);
						break;
				}
			} else
				fPlaybackManager->SetCurrentFrame(0);
		}
		fPlaybackManager->Unlock();
	}
}

// Rewind
void
PlaybackController::Rewind()
{
}

// Forward
void
PlaybackController::Forward()
{
}

// SkipBackward
void
PlaybackController::SkipBackward()
{
	if (fPlaybackManager && fPlaybackManager->Lock()) {
		fPlaybackManager->SetCurrentFrame(0);
		fPlaybackManager->Unlock();
	}
}

// SkipForward
void
PlaybackController::SkipForward()
{
}

// SetVolume
void
PlaybackController::VolumeChanged(float percent)
{
	if (fPlaybackManager && fPlaybackManager->Lock()) {
		fPlaybackManager->SetVolume(percent);
		fPlaybackManager->Unlock();
	}
}

// ToggleMute
void
PlaybackController::ToggleMute(bool mute)
{
	if (mute)
		SetVolume(0.0);
	else
		SetVolume(Volume());
}

// #pragma mark -

void
PlaybackController::ObjectChanged(const Observable* object)
{
	if (object == fCurrentFrame) {

		if (fCurrentFrame->Frame() != fLastCurrentFrame) {
			// the current frame was changed by someone else
			// (if we changed it in response to receiving
			// updates from the playback framework, fLastCurrentFrame
			// would already be up2date)
			if (fPlaybackManager->LockWithTimeout(5000L) >= B_OK) {
				if (fPlaybackManager->IsPlaying()) {
					fPlaybackManager->PausePlaying();
					fPlayAfterDragging = true;
				}
				fLastCurrentFrame = fCurrentFrame->Frame();
				fPlaybackManager->SetCurrentFrame(fLastCurrentFrame);
	
				fPlaybackManager->Unlock();
			}
		}
	
		SetPlaybackFrame(fLastCurrentFrame);
	
		if (!fCurrentFrame->BeingDragged() && fPlayAfterDragging) {
			TogglePlaying();
			fPlayAfterDragging = false;
		}

	} else if (object == fPlaybackRange) {

		BMessage message(MSG_PLAYBACK_SET_RANGE);
		message.AddInt64("start frame", fPlaybackRange->FirstFrame());
		message.AddInt64("end frame", fPlaybackRange->LastFrame());
		fPlaybackManager->PostMessage(&message);

	} else if (object == fDisplayRange) {

		BMessage message(MSG_PLAYBACK_SET_VISIBLE);
		message.AddInt64("start frame", fDisplayRange->FirstFrame() + 1);
		message.AddInt64("end frame", fDisplayRange->LastFrame());
		fPlaybackManager->PostMessage(&message);

	} else if (object == fLoopMode) {

		BMessage message(MSG_PLAYBACK_SET_LOOP_MODE);
		message.AddInt32("mode", fLoopMode->Mode());
		fPlaybackManager->PostMessage(&message);

	}
}

// #pragma mark -

// MessageReceived
void
PlaybackController::MessageReceived(BMessage* message)
{
//printf("PlaybackController::MessageReceived()\n");
	switch (message->what) {
		// playback manager
		case MSG_PLAYBACK_FPS_CHANGED: {
			float fps;
			if (message->FindFloat("fps", &fps) == B_OK) {
				// TODO: ...
			}
			break;
		}
		case MSG_PLAYBACK_MOVIE_BOUNDS_CHANGED: {
			BRect bounds;
			if (message->FindRect("movie bounds", &bounds) == B_OK) {
				// TODO: ...
			}
			break;
		}
		case MSG_PLAYBACK_PLAY_MODE_CHANGED: {
			int32 mode;
			if (message->FindInt32("play mode", &mode) == B_OK) {
				switch (mode) {
					case MODE_PLAYING_FORWARD:
					case MODE_PLAYING_BACKWARD:
						SetPlaybackState(PLAYBACK_STATE_PLAYING);
						break;
					case MODE_PLAYING_PAUSED_FORWARD:
					case MODE_PLAYING_PAUSED_BACKWARD:
						SetPlaybackState(PLAYBACK_STATE_PAUSED);
						break;
					default:
						break;
				}
			}
			break;
		}
		case MSG_PLAYBACK_LOOP_MODE_CHANGED: {
			int32 mode;
			if (message->FindInt32("loop mode", &mode) == B_OK) {
				// TODO: ...
			}
			break;
		}
		case MSG_PLAYBACK_LOOPING_ENABLED_CHANGED:
		{
			bool enabled;
			if (message->FindBool("looping enabled", &enabled) == B_OK) {
				// TODO: ...
			}
			break;
		}
		case MSG_PLAYBACK_CURRENT_FRAME_CHANGED: {
			double currentFrame;
			if (message->FindDouble("current frame", &currentFrame) == B_OK) {
				fLastCurrentFrame = (int64)currentFrame;
				if (!fCurrentFrame->BeingDragged())
					fCurrentFrame->SetFrame(fLastCurrentFrame);
			}
			break;
		}
		case MSG_PLAYBACK_SPEED_CHANGED: {
			float speed;
			if (message->FindFloat("speed", &speed) == B_OK) {
				// TODO: ...
			}
			break;
		}
		case MSG_PLAYBACK_FRAME_DROPPED:
				// TODO: ...
			break;

		default:
			TransportControlGroup::MessageReceived(message);
			break;
	}
}

// SetPlaybackManager
void
PlaybackController::SetPlaybackManager(PlaybackManager* manager)
{
	if (fPlaybackManager && fPlaybackListener)
		fPlaybackManager->RemoveListener(fPlaybackListener);

	fPlaybackManager = manager;

	if (fPlaybackManager && fPlaybackListener)
		fPlaybackManager->AddListener(fPlaybackListener);
}

// SetPlaybackRange
void
PlaybackController::SetPlaybackRange(DisplayRange* range)
{
	if (fPlaybackRange)
		fPlaybackRange->RemoveObserver(this);

	fPlaybackRange = range;

	if (fPlaybackRange) {
		fPlaybackRange->AddObserver(this);
		ObjectChanged(fPlaybackRange);
	}
}

// SetDisplayRange
void
PlaybackController::SetDisplayRange(DisplayRange* range)
{
	if (fDisplayRange)
		fDisplayRange->RemoveObserver(this);

	fDisplayRange = range;

	if (fDisplayRange) {
		fDisplayRange->AddObserver(this);
		ObjectChanged(fDisplayRange);
	}
}

// SetLoopMode
void
PlaybackController::SetLoopMode(LoopMode* mode)
{
	if (fLoopMode)
		fLoopMode->RemoveObserver(this);

	fLoopMode = mode;

	if (fLoopMode) {
		fLoopMode->AddObserver(this);
		ObjectChanged(fLoopMode);
	}
}

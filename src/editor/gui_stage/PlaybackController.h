/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYBACK_CONTROLLER_H
#define PLAYBACK_CONTROLLER_H

#include "Observer.h"
#include "TransportControlGroup.h"

class CurrentFrame;
class DisplayRange;
class LoopMode;
class PlaybackManager;
class PlaybackLOAdapter;

class PlaybackController : public TransportControlGroup,
						   public Observer {
 public:
								PlaybackController(BRect frame,
												   CurrentFrame* currentFrame);
	virtual						~PlaybackController();

	// TransportControlGroup interface
	virtual	void				AttachedToWindow();
	virtual	void				Draw(BRect updateRect);

	virtual	void				TogglePlaying();
	virtual	void				Stop();
	virtual	void				Rewind();
	virtual	void				Forward();
	virtual	void				SkipBackward();
	virtual	void				SkipForward();
	virtual	void				VolumeChanged(float percent);
	virtual	void				ToggleMute(bool mute);

	// Observer
	virtual	void				ObjectChanged(const Observable* object);

	// PlaybackController
	virtual	void				MessageReceived(BMessage* message);

			void				SetPlaybackManager(PlaybackManager* manager);
			void				SetPlaybackRange(DisplayRange* range);
			void				SetDisplayRange(DisplayRange* range);
			void				SetLoopMode(LoopMode* mode);

 private:
			PlaybackManager*	fPlaybackManager;
			PlaybackLOAdapter*	fPlaybackListener;
			CurrentFrame*		fCurrentFrame;
			DisplayRange*		fPlaybackRange;
			DisplayRange*		fDisplayRange;
			LoopMode*			fLoopMode;
			int64				fLastCurrentFrame;
			bool				fPlayAfterDragging;
};


#endif	// PLAYBACK_CONTROLLER_H

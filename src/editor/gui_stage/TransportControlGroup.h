/*
 * Copyright 2006-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef TRANSPORT_CONTROL_GROUP_H
#define TRANSPORT_CONTROL_GROUP_H


// NOTE: Based on my code in the BeOS interface for the VLC media player
// that I did during the VLC 0.4.3 - 0.4.6 times. Code not written by me
// removed. -Stephan Aßmus


#include <GroupView.h>


class BShape;
class DurationView;
class PeakView;
class PlaybackFrameView;
class PlayPauseButton;
class SeekSlider;
class SymbolButton;
class VolumeSlider;


enum {
	PLAYBACK_STATE_PLAYING = 0,
	PLAYBACK_STATE_PAUSED,
	PLAYBACK_STATE_STOPPED,
};

enum {
	SKIP_BACK_ENABLED		= 1 << 0,
	SEEK_BACK_ENABLED		= 1 << 1,
	PLAYBACK_ENABLED		= 1 << 2,
	SEEK_FORWARD_ENABLED	= 1 << 3,
	SKIP_FORWARD_ENABLED	= 1 << 4,
	VOLUME_ENABLED			= 1 << 5,
	SEEK_ENABLED			= 1 << 6,
};


class TransportControlGroup : public BGroupView {
public:
								TransportControlGroup(BRect frame,
									bool usePlaybackFrame, bool useSkipButtons,
									bool usePeakView, bool useWindButtons,
									bool useSeekSlider);
	virtual						~TransportControlGroup();

	// BView interface
	virtual	void				AttachedToWindow();
	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	void				MessageReceived(BMessage* message);

	// TransportControlGroup
	virtual	uint32				EnabledButtons();
	virtual	void				TogglePlaying();
	virtual	void				Stop();
	virtual	void				Rewind();
	virtual	void				Forward();
	virtual	void				SkipBackward();
	virtual	void				SkipForward();
	virtual	void				VolumeChanged(float value);
	virtual	void				ToggleMute(bool mute);
	virtual	void				PositionChanged(float value);

			void				SetEnabled(bool enable);
			void				SetEnabled(uint32 whichButtons);

			void				SetPlaybackState(uint32 state);
			void				SetSkippable(bool backward, bool forward);

			void				SetAudioEnabled(bool enable);
			void				SetMuted(bool mute);
			bool				IsMuted() const;

			void				SetVolume(float percent);
			float				Volume() const;

			void				SetAudioChannelCount(int32 count);

			void				SetPlaybackSpeed(float fps);
			void				SetPlaybackFrame(int64 frame);

			void				SetPosition(float value, bigtime_t position,
									bigtime_t duration);
			float				Position() const;

			PeakView*			GetPeakView() const
									{ return fPeakView; }

			void				SetDisabledString(const char* string);
			void				SetSymbolScale(float scale);

private:
			void				_TogglePlaying();
			void				_Stop();
			void				_Rewind();
			void				_Forward();
			void				_SkipBackward();
			void				_SkipForward();
			void				_UpdateVolume();
			void				_ToggleMute();
			void				_UpdatePosition();

			BShape*				_CreateSkipBackwardsShape(float height) const;
			BShape*				_CreateSkipForwardShape(float height) const;
			BShape*				_CreateRewindShape(float height) const;
			BShape*				_CreateForwardShape(float height) const;
			BShape*				_CreatePlayShape(float height) const;
			BShape*				_CreatePauseShape(float height) const;
			BShape*				_CreateStopShape(float height) const;
			BShape*				_CreateSpeakerShape(float height) const;

private:
			SeekSlider*			fSeekSlider;
			PeakView*			fPeakView;
			PlaybackFrameView*	fPlaybackFrame;
			VolumeSlider*		fVolumeSlider;

			SymbolButton*		fSkipBack;
			SymbolButton*		fSkipForward;
			SymbolButton*		fRewind;
			SymbolButton*		fForward;
			PlayPauseButton*	fPlayPause;
			SymbolButton*		fStop;
			SymbolButton*		fMute;

			BGroupLayout*		fSeekLayout;
			BGroupLayout*		fControlLayout;

			float				fSymbolScale;
};

#endif	// TRANSPORT_CONTROL_GROUP_H

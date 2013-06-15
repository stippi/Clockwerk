/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PLAYER_PLAYBACK_NAVIGATOR_H
#define PLAYER_PLAYBACK_NAVIGATOR_H

#include "PlaybackNavigator.h"

class Playlist;
class RWLocker;
class Schedule;
class ServerObjectManager;

class PlayerPlaybackNavigator : public PlaybackNavigator {
public:
								PlayerPlaybackNavigator(RWLocker* locker);
	virtual						~PlayerPlaybackNavigator();

	virtual	void				Navigate(const NavigationInfo* info);

	// PlayerPlaybackNavigator
			void				SetObjectManager(ServerObjectManager* manager);
			void				SetSchedule(Schedule* schedule);
			Playlist*			MasterPlaylist() const
									{ return fMasterPlaylist; }

			void				SetCurrentFrame(double frame, bool& wasPlaying,
									bool& isPlaying);
			double				CurrentFrame() const
									{ return fCurrentFrame; }
private:
			void				_CleanupTrack(Playlist* playlist,
									uint32 track, int64 startFrame);
			bool				_AddPlaylistToMasterPlaylist(
									Playlist* playlist, uint32 track,
									int64 startFrame, int64 duration);

			RWLocker*			fLocker;
			ServerObjectManager* fObjectManager;
			Schedule*			fSchedule;
			Playlist*			fMasterPlaylist;
				// the master playlist being played,
				// scheduled playlists are added and removed
				// on the fly - removes the necessity to switch
				// playlists in the SimplePlaybackManager
			Playlist*			fPreviousPlaylist;
			Playlist*			fCurrentPlaylist;
			int32				fCurrentScheduleIndex;
				// just some pointer so we know if we are playing

			double				fCurrentFrame;

			Playlist*			fPreviousNavigationPlaylist;
			Playlist*			fNavigationPlaylist;
			int64				fNavigationPlaylistStartFrame;

private:
			class NavigatorEvent;
			friend class NavigatorEvent;

			void				_NavigationTimeout();
			NavigatorEvent*		fNavigationEvent;
};


#endif // PLAYER_PLAYBACK_NAVIGATOR_H

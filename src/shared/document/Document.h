/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "CommandStack.h"
#include "CurrentFrame.h"
#include "DisplayRange.h"
#include "LoopMode.h"
#include "PropertyObject.h"
#include "RWLocker.h"
#include "Selection.h"
#include "VideoViewSelection.h"

struct entry_ref;

class Playlist;

class Document : public RWLocker,
				 public PropertyObject {
 public:
								Document();
	virtual						~Document();

		
			void				SetPlaylist(::Playlist* playlist);
			::Playlist*			Playlist() const
									{ return fPlaylist; }

			::CommandStack*		CommandStack()
									{ return &fCommandStack; }

			::CurrentFrame*		CurrentFrame()
									{ return &fCurrentFrame; }

			::DisplayRange*		DisplayRange()
									{ return &fDisplayRange; }
			::DisplayRange*		PlaybackRange()
									{ return &fPlaybackRange; }
			::LoopMode*			LoopMode()
									{ return &fLoopMode; }

			::Selection*		ClipSelection()
									{ return &fClipSelection; }
			::Selection*		PlaylistSelection()
									{ return &fPlaylistSelection; }
			::VideoViewSelection* VideoViewSelection()
									{ return &fVideoViewSelection; }

			void				SetName(const char* name);
			const char*			Name() const;

			void				SetRef(const entry_ref& ref);
			const entry_ref*	Ref() const
									{ return fRef; }

			void				MakeEmpty();

 private:
			::Playlist*			fPlaylist;
			::CommandStack		fCommandStack;
			::CurrentFrame		fCurrentFrame;
			::DisplayRange		fDisplayRange;
			::DisplayRange		fPlaybackRange;
			::LoopMode			fLoopMode;

			::Selection			fClipSelection;
			::Selection			fPlaylistSelection;
			::VideoViewSelection fVideoViewSelection;

			entry_ref*			fRef;
};

#endif // DOCUMENT_H

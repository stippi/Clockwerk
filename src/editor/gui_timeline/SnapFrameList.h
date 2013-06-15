/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SNAP_FRAME_LIST_H
#define SNAP_FRAME_LIST_H

#include <List.h>

class Playlist;

class SnapFrameList {
 public:
								SnapFrameList();
	virtual						~SnapFrameList();

			void				CollectSnapFrames(const Playlist* list,
									uint64 snapToEndOffset);

			void				AddSnapFrame(int64 frame,
									uint64 snapToEndOffset);

			int64				ClosestFrameFor(int64 frame,
									uint32 track,  double zoomLevel) const;

			int64				ClosestSnapFrameFor(int64 frame) const;
			int64				ClosestSnapFrameBackwardsFor(int64 frame) const;
			int64				ClosestSnapFrameForwardFor(int64 frame) const;

 private:
			void				_MakeEmpty();

			BList				fSnapFrames;
};

#endif // SNAP_FRAME_LIST_H

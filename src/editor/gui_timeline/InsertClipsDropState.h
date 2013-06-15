/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INSERT_CLIPS_DROP_STATE_H
#define INSERT_CLIPS_DROP_STATE_H

#include "DropAnticipationState.h"
#include "SnapFrameList.h"

class TimelineView;

class InsertClipsDropState : public DropAnticipationState {
 public:
								InsertClipsDropState(TimelineView* view);
	virtual						~InsertClipsDropState();

	// ViewState interface
	virtual	void				Draw(BView* into, BRect updateRect);

	// DropAnticipationState interface
	virtual	bool				WouldAcceptDragMessage(const BMessage* dragMessage);
	virtual	Command*			HandleDropMessage(BMessage* dropMessage);
	virtual	void				UpdateDropIndication(const BMessage* dragMessage,
													 BPoint where, uint32 modifiers);
 private:
			TimelineView*		fView;

			SnapFrameList		fSnapFrames;

			int64				fDropFrame;
			uint32				fDropTrack;
			uint64				fDraggedClipDuration;
};

#endif // INSERT_CLIPS_DROP_STATE_H

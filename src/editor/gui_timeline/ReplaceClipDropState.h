/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REPLACE_CLIP_DROP_STATE_H
#define REPLACE_CLIP_DROP_STATE_H

#include "DropAnticipationState.h"

class ClipPlaylistItem;
class TimelineView;

class ReplaceClipDropState : public DropAnticipationState {
 public:
								ReplaceClipDropState(TimelineView* view);
	virtual						~ReplaceClipDropState();

	// ViewState interface
	virtual	void				Draw(BView* into, BRect updateRect);

	// DropAnticipationState interface
	virtual	bool				WouldAcceptDragMessage(const BMessage* dragMessage);
	virtual	Command*			HandleDropMessage(BMessage* dropMessage);
	virtual	void				UpdateDropIndication(const BMessage* dragMessage,
													 BPoint where, uint32 modifiers);
 private:
			TimelineView*		fView;

			ClipPlaylistItem*	fTargetItem;
};

#endif // REPLACE_CLIP_DROP_STATE_H

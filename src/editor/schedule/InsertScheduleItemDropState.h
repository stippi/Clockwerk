/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef INSERT_SCHEDULE_ITEM_DROP_STATE_H
#define INSERT_SCHEDULE_ITEM_DROP_STATE_H

#include "DropAnticipationState.h"

class ScheduleView;

class InsertScheduleItemDropState : public DropAnticipationState {
 public:
								InsertScheduleItemDropState(ScheduleView* view);
	virtual						~InsertScheduleItemDropState();

	// ViewState interface
	virtual	void				Draw(BView* into, BRect updateRect);

	// DropAnticipationState interface
	virtual	bool				WouldAcceptDragMessage(
									const BMessage* dragMessage);
	virtual	Command*			HandleDropMessage(BMessage* dropMessage);
	virtual	void				UpdateDropIndication(
									const BMessage* dragMessage,
									BPoint where, uint32 modifiers);
 private:
			ScheduleView*		fView;

			uint64				fDropFrame;
			uint64				fDraggedPlaylistDuration;
			bool				fInsertAnywhere;
};

#endif // INSERT_SCHEDULE_ITEM_DROP_STATE_H

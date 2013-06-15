/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef DROP_ANTICIPATION_STATE_H
#define DROP_ANTICIPATION_STATE_H

#include <Message.h>

#include "ViewState.h"

class DropAnticipationState : public ViewState {
 public:
								DropAnticipationState(StateView* view);
	virtual						~DropAnticipationState();

	// ViewState interface
	virtual	void				Init();
	virtual	void				Cleanup();

	virtual	void				Draw(BView* into, BRect updateRect);
	virtual	bool				MessageReceived(BMessage* message,
												Command** _command);

	virtual	void				MouseMoved(BPoint where,
										   uint32 transit,
										   const BMessage* dragMessage);
	virtual	Command*			MouseUp();

	virtual	void				ModifiersChanged(uint32 modifiers);

	// DropAnticipationState
	virtual	bool				WouldAcceptDragMessage(const BMessage* dragMessage);
	virtual	Command*			HandleDropMessage(BMessage* dropMessage);
	virtual	void				UpdateDropIndication(const BMessage* dragMessage,
													 BPoint where, uint32 modifiers);

			void				SetDropAnticipationRect(BRect rect);
			void				RemoveDropAnticipationRect();

 protected:
			void				_SetMessage(const BMessage* message);
			void				_UnsetMessage();

			BMessage			fDragMessageClone;
			BRect				fDropAnticipationRect;
};

#endif // DROP_ANTICIPATION_STATE_H

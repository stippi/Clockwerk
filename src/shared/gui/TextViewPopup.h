/*
 * Copyright 2006-2007, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TEXT_VIEW_POPUP_H
#define TEXT_VIEW_POPUP_H

#include <Messenger.h>
#include <Rect.h>
#include <String.h>

class BHandler;
class BMessage;
class BWindow;
class PopupTextView;

class TextViewPopup {
 public:
								TextViewPopup(BRect frame,
											  const BString& text,
											  BMessage* message,
											  BHandler* target);

			void				Cancel();

 private:
	friend class PopupTextView;

	virtual						~TextViewPopup();

			void				TextEdited(const char* text,
										   int32 next = 0);

			PopupTextView*		fTextView;
			BWindow*			fPopupWindow;
			BMessage*			fMessage;
			BMessenger			fTarget;
};

#endif // TEXT_VIEW_POPUP_H

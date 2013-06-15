/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef STATUS_OUTPUT_VIEW_H
#define STATUS_OUTPUT_VIEW_H

#include <GroupView.h>

#include "StatusOutput.h"

class BScrollView;
class BTextView;

class StatusOutputView : public BGroupView, public StatusOutput {
public:
								StatusOutputView(
									const char* initialMessage = NULL);

	virtual void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);

	virtual void				InternalPrintStatusMessage(uint32 type,
									const char* message);

			void				Clear(const char* initialMessage = NULL);

private:
			BScrollView*		fScrollView;
			BTextView*			fTextView;
};


#endif // STATUS_OUTPUT_VIEW_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PROGRESS_REPORTER_H
#define PROGRESS_REPORTER_H


#include <SupportDefs.h>


class ProgressReporter {
public:
								ProgressReporter();
	virtual						~ProgressReporter();

	virtual	void				SetProgressTitle(const char* title) = 0;
	virtual	void				ReportProgress(float percentCompleted,
									const char* label = NULL) = 0;
};


#endif // PROGRESS_REPORTER_H

/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EXECUTE_CLIP_H
#define EXECUTE_CLIP_H

#include <String.h>

#include "Clip.h"
#include "Font.h"

class StringProperty;

class ExecuteClip : public Clip {
 public:
								ExecuteClip(const char* name = NULL);
								ExecuteClip(const ExecuteClip& other);
	virtual						~ExecuteClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	// ExecuteClip
			const char*			CommandPath() const;

 private:
			StringProperty*		fCommandPath;
};

#endif // EXECUTE_CLIP_H

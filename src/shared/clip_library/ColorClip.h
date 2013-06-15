/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COLOR_CLIP_H
#define COLOR_CLIP_H

#include <GraphicsDefs.h>

#include "Clip.h"

class ColorProperty;

class ColorClip : public Clip {
 public:
								ColorClip(const char* name = NULL);
								ColorClip(const ColorClip& other);
	virtual						~ColorClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	// ColorClip
			rgb_color			Color() const;

 private:
			ColorProperty*		fColor;
};

#endif // COLOR_CLIP_H

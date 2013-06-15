/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIMER_RENDERER_H
#define TIMER_RENDERER_H

#include <String.h>

#include "ClipRenderer.h"
#include "Font.h"

class TimerClip;

class TimerRenderer : public ClipRenderer {
 public:
								TimerRenderer(
									ClipPlaylistItem* item,
									TimerClip* clip);
	virtual						~TimerRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	void				Sync();

 private:
			TimerClip*			fClip;

			Font				fFont;
			rgb_color			fColor;
			uint32				fTimeFormat;
			uint32				fTimerDirection;
};

#endif // TIMER_RENDERER_H
